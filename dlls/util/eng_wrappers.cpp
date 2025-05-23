#include "extdll.h"
#include "util.h"
#include "CBaseMonster.h"
#include "pm_materials.h"
#include "pm_shared.h"
#include "eng_wrappers.h"
#include "TextMenu.h"
#include "PluginManager.h"
#include <fstream>

StringSet g_precachedModels;
StringSet g_missingModels;
HashMap<int> g_precachedSounds;
StringSet g_precachedGeneric;
HashMap<int> g_precachedEvents;
StringSet g_tryPrecacheModels;
StringSet g_tryPrecacheSounds;
StringSet g_tryPrecacheGeneric;
StringSet g_tryPrecacheEvents;
StringSet g_precachedSpr;
StringSet g_precachedMdl;

string_t g_indexModels[MAX_PRECACHE]; // maps an index to a model name
string_t g_indexSounds[MAX_PRECACHE]; // maps an index to a model name

HashMap<string_t> g_allocedStrings;

Bsp g_bsp;

void LoadBsp() {
	std::string relPath = std::string("maps/") + STRING(gpGlobals->mapname) + ".bsp";
	std::string mapPath = getGameFilePath(relPath.c_str());
	g_bsp.load_lumps(mapPath);

	if (g_bsp.textures) {
		for (int i = 0; i < g_bsp.textureCount; i++) {
			int32_t texOffset = ((int32_t*)g_bsp.textures)[i + 1];

			if (texOffset == -1) {
				continue;
			}

			BSPMIPTEX& tex = *((BSPMIPTEX*)(g_bsp.textures + texOffset));

			char* texName = tex.szName;
			int slen = strlen(texName);

			if (slen > 1) {
				if (texName[0] == '{' || texName[0] == '!') {
					texName = texName + 1;
				}
				else if (slen > 2 && (texName[0] == '+' || texName[0] == '-')) {
					texName = texName + 2;
				}
			}

			switch (PM_FindTextureType(texName)) {
			case CHAR_TEX_CONCRETE: g_textureStats.tex_concrete = true; break;
			case CHAR_TEX_METAL: g_textureStats.tex_metal = true; break;
			case CHAR_TEX_DIRT: g_textureStats.tex_dirt = true; break;
			case CHAR_TEX_VENT: g_textureStats.tex_duct = true; break;
			case CHAR_TEX_GRATE: g_textureStats.tex_grate = true; break;
			case CHAR_TEX_TILE: g_textureStats.tex_tile = true; break;
			case CHAR_TEX_SLOSH: g_textureStats.tex_water = true; break;
			case CHAR_TEX_WOOD: g_textureStats.tex_wood = true; break;
			case CHAR_TEX_COMPUTER: g_textureStats.tex_computer = true; break;
			case CHAR_TEX_GLASS: g_textureStats.tex_glass = true; break;
			case CHAR_TEX_FLESH: g_textureStats.tex_flesh = true; break;
			default: break;
			}
		}
	}

	// check for any leaves that would make swimming sounds if the player entered them
	if (g_bsp.leaves) {
		for (int i = 0; i < g_bsp.leafCount; i++) {
			int contents = g_bsp.leaves[i].nContents;
			if (contents <= CONTENTS_WATER && contents > CONTENTS_TRANSLUCENT) {
				g_textureStats.tex_water = true;
			}
		}
	}
}

void PRECACHE_DETAIL_TEXTURES() {
	// the engine does this too, but doing it here lets the mod track missing files and overflows
	std::string detail = UTIL_VarArgs("maps/%s_detail.txt", STRING(gpGlobals->mapname));

	std::string detail_path = getGameFilePath(detail.c_str());
	if (detail_path.empty()) {
		return;
	}

	PRECACHE_GENERIC(detail.c_str());

	std::ifstream file(detail_path);
	if (!file.is_open()) {
		return;
	}

	int line_num = 0;
	std::string line;
	while (getline(file, line))
	{
		line_num++;

		line = trimSpaces(line);
		if (line.find("//") == 0 || line.length() == 0)
			continue;

		std::vector<std::string> parts = splitString(line, "\t ");

		if (parts.size() != 4) {
			ALERT(at_warning, "Invalid line in detail file (%d):\n", line_num, detail.c_str());
			continue;
		}

		PRECACHE_GENERIC(("gfx/" + parts[1] + ".tga").c_str());
	}

	file.close();
}

void PRECACHE_HUD_FILES(const char* hudConfigPath) {
	std::string lowerPath = toLowerCase(hudConfigPath);
	hudConfigPath = lowerPath.c_str();

	const char* replacement = g_modelReplacements.get(hudConfigPath);
	if (replacement) {
		hudConfigPath = replacement;
	}

	std::string fpath = getGameFilePath(hudConfigPath);
	std::ifstream infile(fpath);

	if (fpath.empty() || !infile.is_open()) {
		ALERT(at_error, "Failed to load HUD config: %s\n", hudConfigPath);
		return;
	}

	PRECACHE_GENERIC(hudConfigPath);

	int lineNum = 0;
	std::string line;
	while (std::getline(infile, line))
	{
		lineNum++;
		std::string paths[2];

		line = trimSpaces(line);
		if (line.empty()) {
			continue;
		}

		std::vector<std::string> parts = splitString(line, " \t");

		if (parts.size() < 7)
			continue;

		PRECACHE_GENERIC(("sprites/" + parts[2] + ".spr").c_str());
	}
}

int PRECACHE_GENERIC(const char* path) {
	std::string lowerPath = toLowerCase(path);
	path = lowerPath.c_str();

	std::string soundPath = lowerPath;
	if (lowerPath.find("sound/") == 0) {
		soundPath = lowerPath.substr(6);
	}

	const char* replacedModel = g_modelReplacements.get(path);
	const char* replacedSound = g_soundReplacements.get(soundPath.c_str());
	if (replacedModel) {
		path = replacedModel;
	}
	else if (replacedSound) {
		lowerPath = std::string("sound/") + replacedSound;
		path = lowerPath.c_str();
	}

	if (g_serveractive) {
		if (g_precachedGeneric.hasKey(path)) {
			return g_engfuncs.pfnPrecacheGeneric(STRING(ALLOC_STRING(path)));
		}
		else {
			ALERT(at_warning, "PrecacheGeneric failed: %s\n", path);
			return -1;
		}
	}

	if (lowerPath.find(" ") != std::string::npos) {
		// files with spaces causes clients to hang at "Verifying resources"
		// and the file doesn't download
		ALERT(at_error, "Precached file with spaces: '%s'\n", path);
		return -1;
	}

	if (!path || !path[0]) {
		return -1;
	}

	g_tryPrecacheGeneric.put(path);

	if (g_tryPrecacheGeneric.size() < MAX_PRECACHE) {
		g_precachedGeneric.put(path);
		return g_engfuncs.pfnPrecacheGeneric(STRING(ALLOC_STRING(path)));
	}
	else {
		return -1;
	}
}

int PRECACHE_SOUND_ENT(CBaseEntity* ent, const char* path) {
	std::string lowerPath = toLowerCase(path);
	path = lowerPath.c_str();

	path = UTIL_GetReplacementSound(ent ? ent->edict() : NULL, path);

	if (lowerPath.find(" ") != std::string::npos) {
		// files with spaces causes clients to hang at "Verifying resources"
		// and the file doesn't download
		ALERT(at_error, "Precached sound with spaces: '%s'\n", path);
		return g_engfuncs.pfnPrecacheSound(NOT_PRECACHED_SOUND);
	}

	if (g_serveractive) {
		int* precache = g_precachedSounds.get(path);
		if (precache) {
			return *precache;
		}
		else {
			ALERT(at_warning, "PrecacheSound failed: %s\n", path);
			return g_engfuncs.pfnPrecacheSound(NOT_PRECACHED_SOUND);
		}
	}

	if (!path || !path[0]) {
		return g_engfuncs.pfnPrecacheSound(NOT_PRECACHED_SOUND);
	}

	g_tryPrecacheSounds.put(path);

	if (g_tryPrecacheSounds.size() <= MAX_PRECACHE_SOUND) {
		string_t spath = ALLOC_STRING(path);
		int soundIdx = g_engfuncs.pfnPrecacheSound(STRING(spath));
		g_precachedSounds.put(path, soundIdx);
		g_indexSounds[soundIdx] = spath;
		return soundIdx;
	}
	else {
		return g_engfuncs.pfnPrecacheSound(NOT_PRECACHED_SOUND);
	}
}

int PRECACHE_SOUND_NULLENT(const char* path) {
	return PRECACHE_SOUND_ENT(NULL, path);
}

bool validate_mdl(const char* path, studiohdr_t* mdl) {
	if (!path || !mdl) {
		return false;
	}

	// Verify the model is valid
	if (strlen(mdl->name) <= 0) {
		// Ignore T Models being used directly. Maybe it was in a custom_precache entity or something.
		return false;
	}
	if (mdl->id != 1414743113) {
		ALERT(at_error, "Invalid ID in model header: %s\n", path);
		return false;
	}
	if (mdl->version != 10) {
		ALERT(at_error, "Invalid version in model header: %s\n", path);
		return false;
	}
	if (mdl->numseqgroups >= 10000)
	{
		ALERT(at_error, "Too many seqgroups (%d) for model: %s\n", mdl->numseqgroups, path);
		return false;
	}

	return true;
}

void PRECACHE_MODEL_SEQUENCE(const char* path, studiohdr_t* mdl, int sequence) {
	if (sequence >= mdl->numseq) {
		ALERT(at_warning, "Invalid sequence precache %d (max %d) on model %s\n", sequence, mdl->numseq, path);
		return;
	}

	mstudioseqdesc_t* seq = (mstudioseqdesc_t*)((byte*)mdl + mdl->seqindex) + sequence;

	for (int k = 0; k < seq->numevents; k++) {
		mstudioevent_t* evt = (mstudioevent_t*)((byte*)mdl + seq->eventindex) + k;

		std::string opt(evt->options, 64);
		int lastDot = opt.find(".");

		if (lastDot == -1 || lastDot == (int)opt.size() - 1)
			continue; // no file extension

		if (evt->event == 1011 || evt->event == 1004 || evt->event == 1008 || evt->event == 5004) { // play sound
			if (opt[0] == '*')
				opt = opt.substr(1); // not sure why some models do this, it looks pointless.

			if (evt->event == 5004) {
				// sound is loaded by the client on-demand
				PRECACHE_GENERIC(STRING(ALLOC_STRING(normalize_path("sound/" + opt).c_str())));
			}
			else {
				// sound is played by the server
				PRECACHE_SOUND_ENT(NULL, STRING(ALLOC_STRING(opt.c_str())));
			}
		}
		if (evt->event == 5001 || evt->event == 5011 || evt->event == 5021 || evt->event == 5031) { // muzzleflash sprite
			PRECACHE_GENERIC(STRING(ALLOC_STRING(normalize_path(opt).c_str())));
		}
		if (evt->event == 5005) { // custom muzzleflash
			custom_muzzle_flash_t flash = loadCustomMuzzleFlash(opt.c_str());
			if (flash.sprite)
				PRECACHE_MODEL_ENT(NULL, STRING(flash.sprite));
		}
	}
}

void PRECACHE_MODEL_SEQUENCE(const char* path, int sequence) {
	studiohdr_t* mdl = GET_MODEL_PTR(MODEL_INDEX(path));

	if (!validate_mdl(path, mdl)) {
		return;
	}

	PRECACHE_MODEL_SEQUENCE(path, mdl, sequence);
}

void PRECACHE_MODEL_EXTRAS(CBaseEntity* ent, const char* path, studiohdr_t* mdl) {
	if (!validate_mdl(path, mdl)) {
		return;
	}

	// TODO: might want to get the file size from disk to prevent reading invalid memory

	std::string normalizedPath = normalize_path(path);
	std::string pathWithoutExt = normalizedPath;

	int lastDot = pathWithoutExt.find_last_of(".");
	if (lastDot != -1) {
		pathWithoutExt = pathWithoutExt.substr(0, lastDot);
	}

	bool isPlayerModel = normalizedPath.find("models/player/") != std::string::npos;

	// player model preview image
	if (isPlayerModel) {
		const char* bmp = UTIL_VarArgs("%s.bmp", pathWithoutExt.c_str());
		PRECACHE_GENERIC(STRING(ALLOC_STRING(bmp)));
	}

	// external sequence models (01/02/03.mdl)
	if (mdl->numseqgroups > 1) {
		for (int m = 1; m < mdl->numseqgroups; m++) {
			const char* seqmdl = UTIL_VarArgs("%s%02d.mdl", pathWithoutExt.c_str(), m);
			PRECACHE_GENERIC(STRING(ALLOC_STRING(seqmdl)));
		}
	}

	// T model
	if (mdl->numtextures == 0) {
		// Textures aren't used if the model has no triangles
		bool isEmptyModel = true;
		int iGroup = 0;
		mstudiobodyparts_t* bod = (mstudiobodyparts_t*)((byte*)mdl + mdl->bodypartindex) + iGroup;

		for (int i = 0; i < bod->nummodels; i++) {
			mstudiomodel_t* mod = (mstudiomodel_t*)((byte*)mdl + bod->modelindex) + i;

			if (mod->nummesh != 0) {
				isEmptyModel = false;
				break;
			}
		}

		if (!isEmptyModel) {
			const char* tmdl = UTIL_VarArgs("%st.mdl", pathWithoutExt.c_str());
			PRECACHE_GENERIC(STRING(ALLOC_STRING(tmdl)));
		}
	}

	// sounds and sprites attached to events
	for (int i = 0; i < mdl->numseq; i++) {
		if (ent && ent->IsMonster()) {
			mstudioseqdesc_t* seq = (mstudioseqdesc_t*)((byte*)mdl + mdl->seqindex) + i;
			if (seq->activity == 0) {
				// don't precache sequences that the monster doesn't use normally.
				// scripted_sequence will need to precache whichever sequences are needed
				continue;
			}
		}

		PRECACHE_MODEL_SEQUENCE(path, mdl, i);
	}
}

const char* getNotPrecachedModelPath() {
	const char* replacement = g_modelReplacements.get(NOT_PRECACHED_MODEL);
	if (replacement) {
		// TODO: haven't tested if string_t conversion is necessary for the listen server.
		return STRING(ALLOC_STRING(replacement));
	}
	return STRING(ALLOC_STRING(NOT_PRECACHED_MODEL));
}

int PRECACHE_MODEL_ENT(CBaseEntity* ent, const char* path) {
	std::string lowerPath = toLowerCase(path);
	path = lowerPath.c_str();

	// don't track BSP models in MDL hashmaps
	if (path[0] == '*') {
		return g_engfuncs.pfnPrecacheModel(path);
	}

	const char* replacement = g_modelReplacements.get(path);
	if (replacement) {
		path = replacement;
	}

	// loading BSP here because ServerActivate is not soon enough and GameDLLInit is only called once
	if (!g_bsp.loaded) {
		LoadBsp();
	}

	if (lowerPath.find(" ") != std::string::npos) {
		// files with spaces causes clients to hang at "Verifying resources"
		// and the file doesn't download
		ALERT(at_error, "Precached model with spaces: '%s'\n", path);
		return g_engfuncs.pfnPrecacheModel(getNotPrecachedModelPath());
	}

	bool alreadyPrecached = g_precachedModels.hasKey(path);
	if (!alreadyPrecached && getGameFilePath(path).empty()) {
		if (!g_missingModels.hasKey(path)) {
			ALERT(at_error, "Model precache failed. File not found: %s\n", path);
			g_missingModels.put(path);
		}

		return g_engfuncs.pfnPrecacheModel(getNotPrecachedModelPath());
	}

	if (g_serveractive) {
		if (g_precachedModels.hasKey(path)) {
			return g_engfuncs.pfnPrecacheModel(STRING(ALLOC_STRING(path)));
		}
		else {
			ALERT(at_warning, "PrecacheModel failed: %s\n", path);
			return -1;
		}
	}

	if (!path || !path[0]) {
		return g_engfuncs.pfnPrecacheModel(getNotPrecachedModelPath());
	}

	g_tryPrecacheModels.put(path);

	// account for automatic engine precaching of bsp models
	int precachedBspModels = sv_precache_bspmodels->value ? g_bsp.entityBspModelCount : 0;

	if (g_tryPrecacheModels.size() + precachedBspModels < MAX_PRECACHE_MODEL) {
		g_precachedModels.put(path);
		string_t spath = ALLOC_STRING(path);
		int modelIdx = g_engfuncs.pfnPrecacheModel(STRING(spath));
		g_indexModels[modelIdx] = spath;

		std::string pathstr = std::string(path);
		if (pathstr.size() && pathstr.find(".mdl") == pathstr.size() - 4) {
			g_precachedMdl.put(path);
			PRECACHE_MODEL_EXTRAS(ent, path, GET_MODEL_PTR(modelIdx));
		}
		if (pathstr.size() && pathstr.find(".spr") == pathstr.size() - 4) {
			g_precachedSpr.put(path);
		}

		CALL_HOOKS(int, pfnPrecacheModelPost, path);

		return modelIdx;
	}
	else {
		return g_engfuncs.pfnPrecacheModel(getNotPrecachedModelPath());
	}
}

int PRECACHE_REPLACEMENT_MODEL_ENT(CBaseEntity* ent, const char* path) {
	std::string lowerPath = toLowerCase(path);
	path = lowerPath.c_str();

	const char* mapReplacement = g_modelReplacements.get(path);
	const char* modReplacement = g_modelReplacementsMod.get(path);
	if (!mapReplacement) mapReplacement = "";
	if (!modReplacement) modReplacement = "";

	// only precache the model if the model is replaced with something
	// the mod doesn't already replace it with.
	if (!mp_mergemodels.value || strcmp(mapReplacement, modReplacement)) {
		return PRECACHE_MODEL_ENT(ent, path);
	}

	return g_engfuncs.pfnPrecacheModel(getNotPrecachedModelPath());
}

int PRECACHE_MODEL_NULLENT(const char* path) {
	return PRECACHE_MODEL_ENT(NULL, path);
}

int PRECACHE_EVENT(int id, const char* path) {
	std::string lowerPath = toLowerCase(path);
	path = lowerPath.c_str();

	if (g_serveractive) {
		int* existing = g_precachedEvents.get(path);
		if (existing) {
			return *existing;
		}
		else {
			ALERT(at_warning, "PrecacheEvent failed: %s\n", path);
			return -1;
		}
	}

	g_tryPrecacheEvents.put(path);

	if (g_tryPrecacheEvents.size() < MAX_PRECACHE_EVENT) {
		int idx = g_engfuncs.pfnPrecacheEvent(id, STRING(ALLOC_STRING(path)));
		g_precachedEvents.put(path, idx);
		return idx;
	}
	else {
		return -1;
	}
}

bool SET_MODEL(edict_t* edict, const char* model) {
	if (model && model[0] == '*') {
		// BSP model. No special handling.
		CALL_HOOKS(bool, pfnSetModel, edict, model);
		
		if (!g_can_set_bsp_models && !sv_precache_bspmodels->value) {
			// update the Spawn() function if you see this
			ALERT(at_error, "%s set brush model before the bsp model precache phase\n", STRING(edict->v.classname));
			return false;
		}

		g_engfuncs.pfnSetModel(edict, model);
		if (!g_serveractive)
			g_precachedModels.put(model); // engine precaches entity BSP models automatically
		CALL_HOOKS(bool, pfnSetModelPost, edict, model);
		return false;
	}

	std::string lowerPath = toLowerCase(model);
	model = lowerPath.c_str();
	bool replaced = false;

	const char* replacement = g_modelReplacements.get(model);
	if (replacement) {
		model = replacement;
		replaced = true;
	}

	if (!g_precachedModels.hasKey(model)) {
		model = NOT_PRECACHED_MODEL;

		const char* replacement = g_modelReplacements.get(model);
		if (replacement) {
			model = replacement;
		}
	}

	CALL_HOOKS(bool, pfnSetModel, edict, model);

	g_engfuncs.pfnSetModel(edict, model);

	CALL_HOOKS(bool, pfnSetModelPost, edict, model);

	return replaced;
}

bool SET_MODEL_MERGED(edict_t* edict, const char* model, int mergeId) {
	if ((!strcmp(model, MERGED_ITEMS_MODEL) || !SET_MODEL(edict, model)) && mp_mergemodels.value) {
		// save on model slots by using the merged model that contains many different submodels
		SET_MODEL(edict, MERGED_ITEMS_MODEL);
		edict->v.body = mergeId;
		return false;
	}

	return true;
}

const char* GET_MODEL(const char* model) {
	std::string lowerPath = toLowerCase(model);

	const char* replacement = g_modelReplacements.get(lowerPath.c_str());
	if (replacement) {
		model = replacement;
		lowerPath = toLowerCase(model);
	}

	if (!g_precachedModels.hasKey(lowerPath.c_str())) {
		return NOT_PRECACHED_MODEL;
	}

	return model;
}

int MODEL_INDEX(const char* model) {
	std::string lowerPath = toLowerCase(model);
	model = lowerPath.c_str();

	if (!model || model[0] == '\0') {
		return 0;
	}

	const char* replacement = g_modelReplacements.get(model);
	if (replacement) {
		lowerPath = toLowerCase(replacement);
		model = lowerPath.c_str();
	}

	if (!g_precachedModels.hasKey(lowerPath.c_str())) {
		ALERT(at_error, "MODEL_INDEX not precached: %s\n", model);
		return g_engfuncs.pfnModelIndex(getNotPrecachedModelPath());
	}

	return g_engfuncs.pfnModelIndex(model);
}

const char* INDEX_MODEL(int modelIdx) {
	if (modelIdx >= 0 && modelIdx < MAX_PRECACHE) {
		return STRING(g_indexModels[modelIdx]);
	}

	return NOT_PRECACHED_MODEL;
}

int SOUND_INDEX(const char* sound) {
	std::string lowerPath = toLowerCase(sound);
	sound = lowerPath.c_str();

	int* precache = g_precachedSounds.get(lowerPath.c_str());
	if (!precache) {
		ALERT(at_error, "SOUND_INDEX not precached: %s\n", sound);
		int* notPrecached = g_precachedSounds.get(NOT_PRECACHED_SOUND);
		return notPrecached ? *notPrecached : 0;
	}

	return *precache;
}

const char* INDEX_SOUND(int soundIdx) {
	if (soundIdx >= 0 && soundIdx < MAX_PRECACHE) {
		return STRING(g_indexSounds[soundIdx]);
	}

	return NOT_PRECACHED_SOUND;
}


studiohdr_t* GET_MODEL_PTR(edict_t* edict) {
	if (edict->v.model && g_precachedSpr.hasKey(STRING(edict->v.model))) {
		return NULL; // engine crash if getting model ptr for a sprite
	}

	studiohdr_t* header = (studiohdr_t*)g_engfuncs.pfnGetModelPtr(edict);

	if (!header) {
		return NULL;
	}

	return header;
}

studiohdr_t* GET_MODEL_PTR(int modelIdx) {
	if (modelIdx == -1) {
		return NULL;
	}

	// temporarily attach the model to an entity to avoid loading the model from disk again
	edict_t* world = ENT(0);
	int oldModelIdx = world->v.modelindex;
	world->v.modelindex = modelIdx;
	studiohdr_t* ptr = (studiohdr_t*)GET_MODEL_PTR(world);
	world->v.modelindex = oldModelIdx;

	return ptr;
}

void MESSAGE_BEGIN(int msg_dest, int msg_type, const float* pOrigin, edict_t* ed) {
	CALL_HOOKS_VOID(pfnMessageBegin, msg_dest, msg_type, pOrigin, ed);

	TextMenuMessageBeginHook(msg_dest, msg_type, pOrigin, ed);

	if (mp_debugmsg.value) {
		g_lastMsg.msg_dest = msg_dest;
		g_lastMsg.msg_type = msg_type;
		g_lastMsg.hasOrigin = pOrigin != NULL;
		if (pOrigin) {
			memcpy(g_lastMsg.pOrigin, pOrigin, sizeof(float) * 3);
		}
		g_lastMsg.entIdx = ed ? ENTINDEX(ed) : 0;
		g_lastMsg.numMsgParts = 0;
		g_nextStrOffset = 0;
		if (ed)
			strcpy_safe(g_lastMsg.name, STRING(ed->v.netname), 32);
		else
			g_lastMsg.name[0] = 0;
	}

	(*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ed);
}

void WRITE_BYTE(int iValue) {
	CALL_HOOKS_VOID(pfnWriteByte, iValue);
	add_msg_part(MFUNC_BYTE, iValue);
	g_engfuncs.pfnWriteByte(iValue);
}

void WRITE_CHAR(int iValue) {
	CALL_HOOKS_VOID(pfnWriteChar, iValue);
	add_msg_part(MFUNC_CHAR, iValue);
	g_engfuncs.pfnWriteChar(iValue);
}

void WRITE_SHORT(int iValue) {
	CALL_HOOKS_VOID(pfnWriteShort, iValue);
	add_msg_part(MFUNC_SHORT, iValue);
	g_engfuncs.pfnWriteShort(iValue);
}

void WRITE_LONG(int iValue) {
	CALL_HOOKS_VOID(pfnWriteLong, iValue);
	add_msg_part(MFUNC_LONG, iValue);
	g_engfuncs.pfnWriteLong(iValue);
}

void WRITE_ANGLE(float fValue) {
	CALL_HOOKS_VOID(pfnWriteAngle, fValue);
	add_msg_part(MFUNC_ANGLE, fValue);
	g_engfuncs.pfnWriteAngle(fValue);
}

void WRITE_COORD(float fValue) {
	CALL_HOOKS_VOID(pfnWriteCoord, fValue);
	add_msg_part(MFUNC_COORD, fValue);
	g_engfuncs.pfnWriteCoord(fValue);
}

void WRITE_STRING(const char* sValue) {
	CALL_HOOKS_VOID(pfnWriteString, sValue);
	add_msg_part(sValue);
	g_engfuncs.pfnWriteString(sValue);
}

void WRITE_ENTITY(int iValue) {
	CALL_HOOKS_VOID(pfnWriteEntity, iValue);
	add_msg_part(MFUNC_ENTITY, iValue);
	g_engfuncs.pfnWriteEntity(iValue);
}

void MESSAGE_END() {
	CALL_HOOKS_VOID(pfnMessageEnd);
	log_msg(g_lastMsg);
	g_engfuncs.pfnMessageEnd();
}

void EMIT_SOUND_DYN2(edict_t* pEntity, int channel, const char* pszSample, float volume, float attenuation, int fFlags, int pitch) {
	bool sendPAS = channel != CHAN_STATIC && !(fFlags & SND_STOP);
	int hookFlags = fFlags;
	if (!sendPAS) {
		fFlags |= SND_FL_RELIABLE | SND_FL_GLOBAL;
	}
	Vector origin = (pEntity->v.maxs + pEntity->v.mins) * 0.5f + pEntity->v.origin;

	CALL_HOOKS_VOID(pfnEmitSound, pEntity, channel, pszSample, volume, attenuation, hookFlags, pitch, origin, 0);
	
	// TODO: the engine doesn't use these flag bits at all, and yet everyone gets kicked with svc_bad
	// if you pass them to the engine. Destroy an apache to reproduce.
	fFlags &= ~(SND_FL_RELIABLE | SND_FL_GLOBAL);

	g_engfuncs.pfnEmitSound(pEntity, channel, pszSample, volume, attenuation, fFlags, pitch);
}

void EMIT_AMBIENT_SOUND(edict_t* pEntity, const float* vecPos, const char* pszSample, float vol, float attenuation, int fFlags, int pitch) {
	CALL_HOOKS_VOID(pfnEmitAmbientSound, pEntity, vecPos, pszSample, vol, attenuation, fFlags, pitch);
	g_engfuncs.pfnEmitAmbientSound(pEntity, vecPos, pszSample, vol, attenuation, fFlags, pitch);
}

void SetClientMaxspeed(const edict_t* pEntity, float maxspeed) {
	CALL_HOOKS_VOID(pfnSetClientMaxspeed, pEntity, maxspeed);
	g_engfuncs.pfnSetClientMaxspeed(pEntity, maxspeed);
}

void SetClientKeyValue(int clientIndex, char* pszInfoBuffer, const char* pszKey, const char* pszValue) {
	CALL_HOOKS_VOID(pfnSetClientKeyValue, clientIndex, pszInfoBuffer, pszKey, pszValue);
	g_engfuncs.pfnSetClientKeyValue(clientIndex, pszInfoBuffer, pszKey, pszValue);
}

const char* CMD_ARGV(int argc) {
	CALL_HOOKS(const char*, pfnCmd_Argv, argc);
	return g_engfuncs.pfnCmd_Argv(argc);
}

int CMD_ARGC() {
	CALL_HOOKS(int, pfnCmd_Argc);
	return g_engfuncs.pfnCmd_Argc();
}

const char* CMD_ARGS() {
	CALL_HOOKS(const char*, pfnCmd_Args);
	return g_engfuncs.pfnCmd_Args();
}

void CHANGE_LEVEL(const char* pszLevelName, const char* pszLandmarkName) {
	CALL_HOOKS_VOID(pfnChangeLevel, pszLevelName, pszLandmarkName);
	g_engfuncs.pfnChangeLevel(pszLevelName, pszLandmarkName);
}

void PLAYBACK_EVENT_FULL(int flags, const edict_t* pInvoker, unsigned short eventindex, float delay, 
	float* origin, float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, 
	int bparam2)
{
	CALL_HOOKS_VOID(pfnPlaybackEvent, flags, pInvoker, eventindex, delay, origin, angles, fparam1, fparam2,
		iparam1, iparam2, bparam1, bparam2);
	g_engfuncs.pfnPlaybackEvent(flags, pInvoker, eventindex, delay, origin, angles, fparam1, fparam2,
		iparam1, iparam2, bparam1, bparam2);
}

string_t ALLOC_STRING(const char* str) {
	string_t* existing = g_allocedStrings.get(str);

	if (existing) {
		return *existing;
	}

	string_t newStr = g_engfuncs.pfnAllocString(str);
	g_allocedStrings.put(str, newStr);

	return newStr;
}

edict_t* FIND_ENTITY_BY_TARGETNAME(edict_t* entStart, const char* pszName) {
	/*
	edict_t* edicts = ENT(0);
	int startAfter = entStart ? ENTINDEX(entStart) : 0;

	for (int e = startAfter + 1; e < gpGlobals->maxEntities; e++)
	{
		edict_t* ed = &edicts[e];
		if (ed->free)
			continue;

		if (!ed->v.targetname)
			continue;

		if (!strcmp(STRING(ed->v.targetname), pszName))
			return ed;

	}
	return &edicts[0];
	*/
	return FIND_ENTITY_BY_STRING(entStart, "targetname", pszName);
}

EXPORT bool Voice_GetClientListening(int receiver, int sender) {
	CALL_HOOKS(bool, pfnVoice_GetClientListening, receiver, sender);
	return g_engfuncs.pfnVoice_GetClientListening(receiver, sender);
}