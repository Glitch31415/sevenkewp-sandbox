import struct, os, subprocess, wave, time, sys, copy, codecs, json, shutil, copy
from collections import OrderedDict
from PIL import Image

nonstandard_audio_formats = ["aiff", "asf", "asx", "au", "dls", "flac", "fsb", "it", "m3u", "mid", "midi", "mod", "mp2", "ogg", "pls", "s3m", "vag", "wax", "wma", "xm", "xma"]

valve_path = 'C:/Games/Steam/steamapps/common/Half-Life/valve'

def parse_keyvalue(line):
	if line.find("//") != -1:
		line = line[:line.find("//")]
		
	quotes = [idx for idx, c in enumerate(line) if c == '"']
	
	if len(quotes) < 4:
		return None
	
	key   = line[quotes[0]+1 : quotes[1]]
	value = line[quotes[2]+1 : quotes[3]]
	
	return (key, value)

def parse_ents(path, ent_text):
	ents = []

	lineNum = 0
	lastBracket = -1
	ent = None
	
	for line in ent_text.splitlines():
		lineNum += 1
		
		if len(line) < 1 or line[0] == '\n':
			continue
			
		if line[0] == '{':
			if lastBracket == 0:
				print("\n%s.bsp ent data (line %d): Unexpected '{'\n" % (path, lineNum));
				continue
			lastBracket = 0

			ent = OrderedDict()

		elif line[0] == '}':
			if lastBracket == 1:
				print("\n%s.bsp ent data (line %d): Unexpected '}'\n" % (path, lineNum));
			lastBracket = 1

			if ent == None:
				continue

			ents.append(ent)
			ent = None

			# a new ent can start on the same line as the previous one ends
			if line.find("{") != -1:
				ent = OrderedDict()
				lastBracket = 0

		elif lastBracket == 0 and ent != None: # currently defining an entity
			keyvalue = parse_keyvalue(line)
			if keyvalue:
				ent[keyvalue[0]] = keyvalue[1]
	
	return ents

def load_entities(bsp_path):
	with open(bsp_path, mode='rb') as f:
		bytes = f.read()
		version = struct.unpack("i", bytes[:4])
		
		offset = struct.unpack("i", bytes[4:4+4])[0]
		length = struct.unpack("i", bytes[8:8+4])[0]
		
		ent_text = bytes[offset:offset+length].decode("utf-8", "ignore")
		
		return parse_ents(bsp_path, ent_text)
	
	print("\nFailed to open %s" % bsp_path)
	return None
	
def get_all_maps(maps_dir):
	all_maps = []
	
	for file in os.listdir(maps_dir):
		if not file.lower().endswith('.bsp'):
			continue
		if '@' in file:
			continue # ignore old/alternate versions of maps (w00tguy's scmapdb content pool)
			
		all_maps.append(file)
		
	return sorted(all_maps, key=lambda v: v.upper())

def get_all_models(models_dir):
	mdl_files = []
	for root, dirs, files in os.walk(models_dir):
		for file in files:
			if file.endswith('.mdl'):
				mdl_files.append(os.path.join(root, file))
	
	return mdl_files
	
def get_all_cfgs(maps_dir):
	all_cfgs = []
	
	for file in os.listdir(maps_dir):
		if not file.lower().endswith('.cfg'):
			continue
			
		all_cfgs.append(file)
		
	return sorted(all_cfgs, key=lambda v: v.upper())

def get_all_skies(skies_dir):
	sky_files = []
	
	if os.path.exists(skies_dir):
		for file in os.listdir(skies_dir):
			if file.endswith('.tga') or file.endswith('.bmp'):
				sky_files.append(os.path.join(skies_dir, file))
	
	return sky_files

def get_all_wavs(sounds_dir):
	wav_files = []
	for root, dirs, files in os.walk(sounds_dir):
		for file in files:
			if file.endswith('.wav'):
				wav_files.append(os.path.join(root, file))
	
	return wav_files

def convert_audio(file, out_format, samp_rate):
	global converted_files
	
	new_file = os.path.splitext(file)[0] + '.' + out_format
	#print("Convert %s -> %s" % (file, new_file))
	
	if file.lower() in converted_files:
		return converted_files[file.lower()]
	
	rename_new = False
	if new_file.lower() == file.lower():
		new_file = os.path.splitext(file)[0] + '_v2.' + out_format
		rename_new = True
	
	# half-life audio is 22kz max, including the mp3 player
	samp_rate = min(22050, samp_rate)
	
	if out_format == 'mp3':
		#samp_rate = max(samp_rate, 11025) # 8000 or less breaks the mp3 player
		samp_rate = 22050
	
	wav = None
	if file.endswith(".wav"):
		cue_points = get_wave_cue_points('sound/' + file)
		try:
			wav = wave.open('sound/' + file, 'rb')
			
			
			if len(cue_points) == 1 and cue_points[0] > wav.getnframes()/2:
				cue_points = [] # just remove them. The loop would not work anyway
				print("Removing invalid cue point")
			elif len(cue_points) == 2 and cue_points[0] > cue_points[1]:
				cue_points = [cue_points[1], cue_points[0]]
				print("Swapping inverted cue points")
			
			wav.close()
		except Exception as e:
			if 'unknown format' not in '%s' % e:
				raise e			
	
	ffmpeg_command = ''
	if out_format == 'wav':
		ffmpeg_command = ["ffmpeg", "-i", 'sound/' + file, '-c:a', 'pcm_u8', '-ac', '1', '-ar', '%d' % samp_rate, "-y", 'sound/' + new_file]
	elif out_format == 'mp3':
		ffmpeg_command = ["ffmpeg", "-i", 'sound/' + file, '-c:a', 'libmp3lame', '-ar', '%d' % samp_rate, '-q:a', '5', "-y", 'sound/' + new_file]
	else:
		print("invalid target format: %s" % out_format)

	print(' '.join(ffmpeg_command))
	pipe = subprocess.run(ffmpeg_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	#print(pipe.stderr.decode("utf-8"))
	
	if not os.path.exists('sound/' + new_file):
		print("Failed to convert")
		return file
	
	if wav and cue_points and out_format == 'wav':		
		new_wav = wave.open('sound/' + new_file, 'rb')
		new_wav.close()
		
		new_cues = []
		
		for cue in cue_points:
			t = cue / wav.getframerate()
			new_t = min(int(t * new_wav.getframerate()), new_wav.getnframes()-1)
			new_cues.append(new_t)
			#print("cue %f = %d -> %d" % (t, cue, new_t))
			
		add_wave_cue_points('sound/' + new_file, new_cues)
	
	while True:
		try:
			if os.path.exists('sound/' + file):
				os.remove('sound/' + file)
			break
		except Exception as e:
			print(e)
			time.sleep(0.5)
			print("Failed to delete file. Waiting...")
	
	if rename_new:
		os.rename('sound/' + new_file, 'sound/' + file)
		new_file = file
	
	converted_files[file.lower()] = new_file
	
	return new_file

def lowercase_rename( dir ):
    # renames all subforders of dir, not including dir itself
    def rename_all( root, items):
        for name in items:
            try:
                os.rename( os.path.join(root, name), os.path.join(root, name.lower()))
            except OSError:
                pass # can't rename it, so what

    # starts from the bottom so paths further up remain valid after renaming
    for root, dirs, files in os.walk( dir, topdown=False ):
        rename_all( root, dirs )
        rename_all( root, files)


def get_wave_cue_points(fpath):
	global converted_files
	
	sample_rate = 0
	bytes_per_sample = 0
	read = 0
	file = open(fpath, "rb")

	file.seek(0, os.SEEK_END)
	fsize = file.tell()
	file.seek(0, os.SEEK_SET)

	# Read RIFF_HEADER
	riff_str = file.read(4)
	overall_size = int.from_bytes(file.read(4), byteorder='little')
	wave_str = file.read(4)

	if riff_str != b'RIFF' or wave_str != b'WAVE':
		print("Invalid WAVE header: {}".format(fpath))
		return []

	# Read other chunks
	while True:
		if (file.tell() + 8 >= fsize):
			break
			
		chunk_name = file.read(4).decode('ascii', 'ignore')
		chunk_size = int.from_bytes(file.read(4), byteorder='little')
		padded_size = int((chunk_size + 1) / 2) * 2

		if chunk_name == "cue ":
			numCues = int.from_bytes(file.read(4), byteorder='little')
			
			cues = []
			
			for x in range(0, numCues):
				id = int.from_bytes(file.read(4), byteorder='little')
				position = int.from_bytes(file.read(4), byteorder='little')
				chunkId = file.read(4).decode('utf-8')
				chunkStart = int.from_bytes(file.read(4), byteorder='little')
				blockStart = int.from_bytes(file.read(4), byteorder='little')
				sampleStart = int.from_bytes(file.read(4), byteorder='little')
				cues.append(sampleStart)
			
			return cues
		else:
			file.seek(padded_size, os.SEEK_CUR)

	return []

def add_wave_cue_points(fpath, cue_points):
	with open(fpath, 'r+b') as file:
		file.seek(0, os.SEEK_END)
		
		file.write(b'cue ')
		file.write(struct.pack('<I', len(cue_points)*24))
		file.write(struct.pack('<I', len(cue_points)))
		
		for idx, cue in enumerate(cue_points):
			file.write(struct.pack('<I', idx))
			file.write(struct.pack('<I', 0)) # position
			file.write(b'data')
			file.write(struct.pack('<I', 0)) # chunk start
			file.write(struct.pack('<I', 0)) # block start
			file.write(struct.pack('<I', cue)) # sample start
			
		newSize = file.tell() - 8 # sub the WAVE header
		
		# update file size
		file.seek(4, os.SEEK_SET)
		file.write(struct.pack('<I', newSize))

converted_files = {}

unique_errors = set()

def err(text):
	if text in unique_errors:
		return
	unique_errors.add(text)
	print('\n%s' % text, end='')

def wav_needs_conversion(soundPath, fix_problems):
	cue_points = get_wave_cue_points(soundPath)
	cue_string = ' CUE' if len(cue_points) else ''

	chans = 0
	samprate = 0
	sampsz = 0
	numsamps = 0

	try:
		wav = wave.open(soundPath, 'rb')
		chans = wav.getnchannels()
		samprate = wav.getframerate()
		sampsz = wav.getsampwidth()
		numsamps = wav.getnframes()
		wav.close()
	except Exception as e:
		if 'unknown format' not in '%s' % e:
			raise e
	
	invalid_format = chans != 1 or samprate > 44100 or sampsz > 2
	bad_format = samprate > 22050 or sampsz > 1 # HL audio is 22050 and you can't tell 8-bit from 16-bit
	fmt_str = 'Invalid' if invalid_format else 'Suboptimal'
	
	bad_cues = False
	mid_point = numsamps*0.5
	if len(cue_points) == 1 and cue_points[0] >= mid_point:
		bad_cues = True
		if not fix_problems:
			err("WAV cue point placed after mid point: %s" % (soundPath))
	if len(cue_points) > 1 and cue_points[0] > cue_points[1]:
		bad_cues = True
		if not fix_problems:
			err("WAV cue points inverted: %s" % (soundPath))
	
	needs_convert = chans != 1 or samprate > 22050 or sampsz > 1 or bad_cues
	
	if not fix_problems and needs_convert:
		err("%s WAV format: %s (%d chan %d %d-bit%s)" % (fmt_str, soundPath, chans, samprate, sampsz*8, cue_string))
	
	return (needs_convert, samprate)

def check_map_problems(all_ents, fix_problems):
	global default_files
	global converted_files
	global modelguy_path
	global wadmaker_path
	global magick_path
	global bspguy_path
	global nonstandard_audio_formats
	global all_titles
	
	any_problems = False
	
	# known scversions (after scanning every scmapdb map):
	# 300 4 401 450 470 471 480 500 502 509-525
	scversion = 200
	for ent in all_ents:
		if ent.get('classname', '') == "worldspawn":
			scversion = int(ent.get("scversion2", ent.get("scversion", "200")))
			if scversion == 4:
				scversion = 400
			
			for key in ent.keys():
				if key.lower() == 'maxrange' and key != 'MaxRange':
					print("MaxRange worldspawn key with bad casing: %s" % key)
					if fix_problems:
						ent["MaxRange"] = ent[key]
						del ent[key]
					else:
						any_problems = True
					break
			for key in ent.keys():
				if key.lower() == 'waveheight' and key != 'WaveHeight':
					print("WaveHeight worldspawn key with bad casing: %s" % key)
					if fix_problems:
						ent["WaveHeight"] = ent[key]
						del ent[key]
					else:
						any_problems = True
					break
			break

	for ent in all_ents:
		cname = ent.get('classname', '')
		
		# lazy way to bulk replace music files in maps
		should_music = [
			#"bm_nightmare/boss_music2.wav",
		]
		
		if cname in ['ambient_music', 'ambient_generic']:
			soundFile = ent.get('message', '').lower()
			
			if 'media/half-life' in soundFile and soundFile.endswith('.mp3'):
				# maps a sven cd track to an HL cd track
				sven_to_hl_cd_tracks = {
					"half-life01.mp3": 2,
					"half-life02.mp3": 3,
					"half-life03.mp3": 4,
					"half-life04.mp3": 5,
					"half-life05.mp3": 6,
					"half-life06.mp3": 7,
					"half-life07.mp3": 8,
					"half-life08.mp3": 9,
					"half-life09.mp3": 10,
					"half-life10.mp3": 11,
					"half-life11.mp3": 12,
					"half-life12.mp3": 13,
					"half-life13.mp3": 14,
					"half-life14.mp3": 15,
					"half-life15.mp3": 16,
					"half-life16.mp3": 17,
					"half-life17.mp3": 18,
					"half-life18.mp3": 19,
					"half-life19.mp3": 20,
					"half-life20.mp3": 21,
					"half-life21.mp3": 22,
					"half-life22.mp3": 23,
					"half-life23.mp3": 24,
					"half-life24.mp3": 25,
					"half-life25.mp3": 26,
					"half-life26.mp3": 27,
					"half-life27.mp3": 28
				}
				
				cdtrack = 0
				for key, val in sven_to_hl_cd_tracks.items():
					if key in soundFile:
						cdtrack = val
						break
				
				if not fix_problems:
					err("%s should use CD audio track %d: %s" % (cname, cdtrack, soundFile))
					any_problems = True
				else:
					ent["classname"] = "target_cdaudio"
					ent["health"] = cdtrack
	
		if cname in ['ambient_music', 'ambient_generic', 'scripted_sentence']:
			spawnflags = int(ent.get("spawnflags", 0))
			is_global = (cname == 'ambient_generic' and (spawnflags & 1)) or cname == 'ambient_music'
			is_music = 'ambient_music' in ent['classname']
			soundFile = ent.get('sentence', '').lower() if cname == 'scripted_sentence' else ent.get('message', '').lower()
			
			if soundFile in should_music:
				is_music = True
			
			prefix = ''
			if soundFile.startswith('+'):
				prefix = '+'
				soundFile = soundFile[1:]
			
			soundPath = os.path.join(maps_dir, '..', 'sound/' + soundFile)
			ext = os.path.splitext(soundFile)[1][1:]
			global_str = 'GLOBAL' if is_global else 'LOCAL'
			
			
			if soundFile.startswith('!'):
				continue # sentence name, not a sound file
			
			if scversion >= 401 and scversion <= 449 and ent.get('playmode', 0) == 0 and not soundFile.endswith('.wav') and (int(ent.get('spawnflags', 0)) & 32) == 0:
				# in early versions of SC 4.x, special audio formats looped if playmode was set to default
				# and if the cylic flagged was unchecked. In later versions, the default is to play once and
				# playmode overrides the cyclic flag.
				if not fix_problems:
					err("Enable old ambient loop behavior: %s" % soundFile)
					any_problems = True
				else:
					ent["playmode"] = "2"
			
			if ' ' in soundFile:
				if not fix_problems:
					err("Sound file contains spaces: %s" % soundFile)
					any_problems = True
				else:
					newName = soundFile.replace(' ', '_')
					ent['message'] = prefix + newName
					if os.path.exists('sound/' + soundFile) and not os.path.exists('sound/' + newName):
						os.rename('sound/' + soundFile, 'sound/' + newName)
					soundFile = newName
			
			if soundFile.lower() in converted_files:
				ent['message'] = prefix + converted_files[soundFile.lower()]
				continue
			
			if not os.path.exists(soundPath):
				if 'sound/' + soundFile.lower() not in default_files:
					err("Missing file: %s\n" % ('sound/' + soundFile))
					#any_problems = True
					pass
				continue
			
			if ext == 'wav':
				needs_convert, samprate = wav_needs_conversion(soundPath, fix_problems)
				
				if is_music:
					if not fix_problems:
						err("WAV used in ambient_music: %s" % (soundFile))
					else:
						ent['message'] = prefix + convert_audio(soundFile, 'mp3', samprate)
					any_problems = True
				if needs_convert:
					if fix_problems:
						ent['message'] = prefix + convert_audio(soundFile, 'wav', samprate)
					any_problems = True
					
			elif ext == 'mp3' and not is_global:
				if not fix_problems:
					err("Local MP3 audio: %s (%s)" % (soundFile, global_str))
				else:
					ent['message'] = prefix + convert_audio(soundFile, 'wav', 22050)
				any_problems = True
			elif ext in nonstandard_audio_formats:
				if not fix_problems:
					err("Nonstandard format: %s (%s)" % (soundFile, global_str))
				else:
					if is_global:
						ent['message'] = prefix + convert_audio(soundFile, 'mp3', 22050)
					else:
						ent['message'] = prefix + convert_audio(soundFile, 'wav', 22050)
				any_problems = True
				
		if cname.startswith("weapon_"):
			if '.mdl' in ent.get('iuser1', '').lower():
				if not fix_problems:
					err("Custom weapon P model in user1: %s (%s)" % (cname, ent.get('iuser1', '')))
				else:
					ent['wpn_p_model'] = ent['iuser1']
					del ent['iuser1']
				any_problems = True
			if '.mdl' in ent.get('iuser2', '').lower():
				if not fix_problems:
					err("Custom weapon W model in user2: %s (%s)" % (cname, ent.get('iuser2', '')))
				else:
					ent['wpn_w_model'] = ent['iuser2']
					del ent['iuser2']
				any_problems = True
			if '.mdl' in ent.get('iuser3', '').lower():
				if not fix_problems:
					err("Custom weapon V model in user3: %s (%s)" % (cname, ent.get('iuser3', '')))
				else:
					ent['wpn_v_model'] = ent['iuser3']
					del ent['iuser3']
				any_problems = True
				
		if cname == "trigger_setorigin":
			spawnflags = int(ent.get('spawnflags', 0))
			if spawnflags == 0:
				if not fix_problems:
					err("trigger_setorigin with no spawnflags set will implicitly enable all copy axis flags")
				else:
					ent['spawnflags'] = "%d" % 896
				any_problems = True
			if (spawnflags & 8) != 0 and (spawnflags & 896) == 0:
				if not fix_problems:
					err("trigger_setorigin with no Lock Offsets flag set, but no copy axis flags set, will implicitly enable all copy axis flags")
				else:
					ent['spawnflags'] = "%d" % (spawnflags | 896)
				any_problems = True
		
		if cname == "trigger_changesky" and ent.get('skyname', ""):
			skyname = ent.get('skyname', "")
			bsppath = 'models/skybox/%s.bsp' % skyname
			
			if not os.path.exists(bsppath) and not fix_problems:
				err("trigger_changesky requires sky to bsp conversion")
				any_problems = True
			elif not os.path.exists(bsppath):
				can_convert = True
				skybox_image_tmp_path = "_skybox/images"
				shutil.rmtree(skybox_image_tmp_path)
				os.makedirs(skybox_image_tmp_path, exist_ok=True)
				sky_suffixes = ["ft", "bk", "lf", "rt", "up", "dn"]
				
				for suffix in sky_suffixes:
					fpath = "gfx/env/%s%s.tga" % (skyname, suffix)
					if not os.path.exists(fpath):
						fpath = os.path.join(valve_path, fpath)
						if not os.path.exists(fpath):
							print("ERROR: Failed to find file for trigger_changesky model: %s" % fpath)
							can_convert = False
							break
					
					cmd = [magick_path, 'convert', '-auto-orient', '-depth', '8', '-type', 'palette', '+dither', fpath, "%s/box_%s.bmp" % (skybox_image_tmp_path, suffix)]
					print(' '.join(cmd))
					subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
						
				if can_convert:
					cmd = [wadmaker_path, '-nologfile', '-full', '_skybox/images', 'skybox.wad']
					print(' '.join(cmd))
					subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
					
					cmd = [ripent_path, '-textureimport', '_skybox/skybox']
					print(' '.join(cmd))
					subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
					
					os.makedirs('models/skybox', exist_ok=True)
					
					shutil.copyfile('_skybox/skybox.bsp', bsppath)
					
					# rename textures because each texture loaded by the client must have a unique name
					# without this you can't use 3+ skies or change levels to another map with trigger_changesky
					shortskyname = skyname[:13] # leave room for 2 char suffix and the null char
					for suffix in sky_suffixes:
						newname = suffix + shortskyname # suffix comes first in case the sky name starts with "sky", which is invisible to the client
						cmd = [bspguy_path, 'renametex', bsppath, '-old', 'box_%s' % suffix, '-new', newname]
						print(' '.join(cmd))
						subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		
		if cname == 'env_message':
			title_name = ent.get("message", "").lower()
		
			if title_name in all_titles:
				if not fix_problems:
					err("env_message using title from titles.txt: %s" % title_name)
					any_problems = True
				else:
					spawnflags = int(ent.get('spawnflags', 0))
					
					title = all_titles[title_name]
					ent['message'] = title["text"].replace('"', "''")
					ent['x'] = title["x"]
					ent['y'] = title["y"]
					ent['fadein'] = title["fadein"]
					ent['fadeout'] = title["fadeout"]
					ent['effect'] = title["effect"]
					ent['color'] = title["color"]
					ent['color2'] = title["color2"]
					ent['holdtime'] = title["holdtime"]
					ent['fxtime'] = title["fxtime"]
					ent['classname'] = 'game_text'
					ent["spawnflags"] = "1" if (spawnflags & 2) else 0
					
					if (spawnflags & 1):
						# kill the text after it plays
						ent["killtarget"] = ent["targetname"]
					
					sound = ent.get("messagesound", "")
					if sound:
						attn = int(ent.get("messageattenuation", "0"))
						ori = ent.get("origin", "")
						ori_parts = ori.split()
						if (len(ori_parts) > 2):
							ori_parts[2] = "%d" % (int(ori_parts[2]) + 24)
							ori = " ".join(ori_parts)
						
						newent = {}
						newent["classname"] = "ambient_generic"
						newent["targetname"] = ent["targetname"]
						newent["origin"] = ori
						newent["message"] = sound
						newent["health"] = ent.get("messagevolume", "0")
						newent["pitch"] = "100"
						
						spawnflags = 16 | 32
						
						if attn == 0:
							spawnflags |= 2
						elif attn == 1:
							spawnflags |= 4
						elif attn == 2:
							spawnflags |= 8
						elif attn == 3:
							spawnflags |= 1
							
						newent["spawnflags"] = "%d" % spawnflags
						
						all_ents.append(newent)
						
		
		if cname == 'func_vehicle_custom':
			if not fix_problems:
				err("func_vehicle_custom used instead of func_vehicle")
				any_problems = True
			else:
				ent["classname"] = "func_vehicle"
		
		# custom models with external sequences cause crashes if the "vanilla" model they're referencing
		# does not exist (e.g. "models/shocktrooper01.mdl" for a custom shocktrooper model)
		for key, value in ent.items():
			value = ('%s' % value).lower()
			if '.mdl' in value:
				if os.path.exists(value) and os.path.exists(value.replace(".mdl", "01.mdl")):
					if not fix_problems:
						err("Model with external sequences: %s (%s)" % (value, cname))
						any_problems = True
					else:
						mdlguy_command = [modelguy_path, 'merge', value]
						print(' '.join(mdlguy_command))
						subprocess.run(mdlguy_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
						
			if ('.mdl' in value or '.wav' in value) and '\\' in value:
				if not fix_problems:
					err("Wrong slash type in file path: %s (%s)" % (value, cname))
					any_problems = True
				else:
					idx = value.find('\\')
					while idx != -1:
						value = value[:idx] + '/' + value[idx+2:]
						idx = value.find('\\')
					ent[key] = value
			
	return any_problems

# map-specific edits
def convert_ents(all_ents):
	for ent in all_ents:
		for key in ['model', 'new_model']:
			model = ent.get(key, '').lower()
			
			if model == 'models/snd/hgrunt_desert.mdl':
				ent[key] = 'models/faraon/hgrunt_desert.mdl'
			if model == 'models/sandstone/rpggruntf.mdl':
				ent[key] = 'models/faraon/hgrunt_desert.mdl'

def get_all_skill_cvar_names(skill_path):	
	all_names = set()

	with open(skill_path, 'r') as file:
		for line in file:
			line = line.strip().lower()
			
			parts = line.split()
			if len(parts) < 2:
				continue
				
			if parts[0].startswith("sk_"):
				all_names.add(parts[0].lower())
				
	return all_names

def convert_skill_cfg(skill_path):
	global all_skill_cvar_names
	new_cvar_lines_idx = {} # maps a cvar line to an index in new_skill_lines
	best_cvar_levels = {}
	new_skill_lines = []
	change_count = 0
	
	with open(skill_path, 'r') as file:
		for line in file:
			line = line.strip().lower()
			
			parts = line.split()
			if len(parts) < 2:
				new_skill_lines.append(line)
				continue
				
			skill_name = parts[0].lower()
			
			suffix = skill_name[-1]
			new_skill_name = skill_name[:-1]
			
			if suffix in ["1", "2", "3"] and new_skill_name in all_skill_cvar_names:
				if new_skill_name in best_cvar_levels:
					# prefer highest skill level setting
					old_best = int(best_cvar_levels[new_skill_name])
					new_best = int(suffix)
					if new_best <= old_best:
						continue
				
				best_cvar_levels[new_skill_name] = suffix
				
				if new_skill_name in new_cvar_lines_idx:
					del new_skill_lines[new_cvar_lines_idx[new_skill_name]]
				
				new_skill_lines.append("%s %s" % (new_skill_name, " ".join(parts[1:])))
				new_cvar_lines_idx[new_skill_name] = len(new_skill_lines)-1
				change_count += 1
				
				continue
			
			new_skill_lines.append(line)

	if change_count > 0:
		with open(skill_path, 'w') as file:
			for line in new_skill_lines:
				file.write(line + "\n")
				
		print("Rewrote/removed %d skill file lines" % change_count)

def parse_titles(titles_path):
	titles = {}
	
	# defaults copied from the client code
	current_hud_params = {
		'color': '100 100 100',
		'color2': '240 110 0',
		'x': '-1',
		'y': '0.7',
		'fadein': '0.01',
		'fadeout': '1.5',
		'fxtime': '0.25',
		'holdtime': '5'
	}
	
	current_hud_text = ''
	title_name = ''
	parsing_hud_text = False

	with open(titles_path, 'r') as file:
		for line in file:
			line = line.strip()
			
			if not line or line.startswith("//"):
				continue
				
			if not parsing_hud_text and line.startswith("$"):
				parts = line.split()
				param_name = parts[0][1:].lower()
				
				if param_name == 'position':
					current_hud_params['x'] = parts[1]
					current_hud_params['y'] = parts[2]
				else:
					param_value = " ".join(parts[1:])
					current_hud_params[param_name] = param_value
			elif line.startswith("{"):
				parsing_hud_text = True
			elif line.startswith("}"):
				parsing_hud_text = False
				current_hud_params['text'] = current_hud_text
				titles[title_name] = copy.deepcopy(current_hud_params)
				current_hud_text = current_hud_params['text'] = ''
			else:
				if parsing_hud_text:
					if current_hud_text:
						current_hud_text += '\n'
					current_hud_text += line
				else:
					title_name = line.lower()
	
	#for key, val in titles.items():
	#	print("%s = %s" % (key, val))
	
	return titles

def parse_sentences(sent_path):
	sentences = {}

	with open(sent_path, 'r') as file:
		for line in file:
			line = line.strip()
			
			if not line or line.startswith("//"):
				continue
				
			parts = line.split()
			
			if len(parts) < 2:
				continue
				
			sent_name = parts[0].lower()
			sent_folder = 'vox'
			
			if '/' in parts[1]:
				sent_folder = parts[1][:parts[1].find("/")]
				parts[1] = parts[1][parts[1].find("/")+1:]
			
			sentences[sent_name] = {}
			sentences[sent_name]["folder"] = sent_folder
			sentences[sent_name]["sounds"] = parts[1:]
			sentences[sent_name]["has_effects"] = True if '(' in line else False
			sentences[sent_name]["line"] = line
		
	return sentences

def write_unique_sentences(hl_sents, sc_sents):
	unique_sents = {}
	
	for key, val in sc_sents.items():
		if key not in hl_sents:
			unique_sents[key] = val
	
	sorted_keys = sorted(unique_sents.keys())
	
	with open("hlcoop_sentences.txt", 'w') as file:
		last_key = ''
		for key in sorted_keys:
			if last_key[:-1] != key[:-1]:
				file.write("\n")
			last_key = key
			file.write(unique_sents[key]["line"] + "\n")
	
	return unique_sents

def ents_match(d1, d2, path=""):
	if len(d1) != len(d2):
		return False

	for idx, ent in enumerate(d1):
		e1 = d1[idx]
		e2 = d2[idx]
	
		for k in e1.keys():
			if k in e2:
				if e1[k] != e2[k]:
					return False
			else:
				return False
		for k in e2.keys():
			if k not in e1:
				return False
	return True

default_files = set()
for line in open('resguy_default_content.txt', 'r').readlines():
	line = line.strip()
	if line == '[DEFAULT FILES]' or not line:
		continue
	if line == '[DEFAULT TEXTURES]':
		break
	default_files.add(line.lower())

cur_dir = os.getcwd()
ripent_path = os.path.join(cur_dir, 'ripent')
modelguy_path = os.path.join(cur_dir, 'modelguy')
wadmaker_path = os.path.join(cur_dir, '_skybox', 'wadmaker')
bspguy_path = os.path.join(cur_dir, 'bspguy')
magick_path = 'magick'
skill_path = os.path.join(cur_dir, "skill.cfg")
titles_path = os.path.join(cur_dir, "titles.txt")
#sent_path_hl = os.path.join(cur_dir, "sentences_hl.txt")
#sent_path_sc = os.path.join(cur_dir, "sentences_sc.txt")

#os.chdir('../compatible_maps')

maps_dir = "maps"
models_dir = "models"

all_maps = get_all_maps(maps_dir)
all_cfgs = get_all_cfgs(maps_dir)
all_models = get_all_models(models_dir)
all_skies = get_all_skies('gfx/env')
all_skill_cvar_names = get_all_skill_cvar_names(skill_path)
all_titles = parse_titles(titles_path)
#sentences_hl = parse_sentences(sent_path_hl)
#sentences_sc = parse_sentences(sent_path_sc)
#write_unique_sentences(sentences_hl, sentences_sc)

fix_problems = True

if fix_problems:
	print("Lowercasing files...", end='')
	lowercase_rename('.')
	print("DONE")

if fix_problems:
	print("Merging external models")
	for mdl in all_models:
		if os.path.exists(mdl.lower().replace(".mdl", "01.mdl")) or os.path.exists(mdl.lower().replace(".mdl", "t.mdl")):
			mdlguy_command = [modelguy_path, 'merge', mdl]
			print(' '.join(mdlguy_command))
			subprocess.run(mdlguy_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	print("Converting model event sounds")
	for mdl in all_models:
		json_path = 'temp.json'
		
		mdlguy_command = [modelguy_path, 'info', mdl, json_path]
		#print(' '.join(mdlguy_command))
		subprocess.run(mdlguy_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		
		if not os.path.exists(json_path):
			print("Zomg failed to info model %s" % mdl)
			continue
		
		had_nonstandard_audio = False
		
		with open(json_path, 'r') as file:
			data = json.load(file)
			
			for tex in  data["textures"]:
				if tex["width"] * tex["height"] > 512*512:
					newWidth = 0
					newHeight = 0
					if tex["width"] > tex["height"]:
						newWidth = 512
						newHeight = (tex["height"] * (512.0 / tex["width"]) + 0.5)
					else:
						newHeight = 512
						newWidth = (tex["width"] * (512.0 / tex["height"]) + 0.5)
					
					# pixel count must be divisble by 8 or else game crashes with "GL_Upload8: s&3"
					newWidth =  int((newWidth + 4) / 8) * 8
					newHeight = int((newHeight + 4) / 8) * 8
					
					print("Resizing invalid texture in %s (%s %dx%d -> %dx%d)" % (mdl, tex["name"], tex["width"], tex["height"], newWidth, newHeight))
					mdlguy_command = [modelguy_path, 'resize', tex["name"], "%dx%d" % (newWidth, newHeight), mdl]
					print(' '.join(mdlguy_command))
					subprocess.run(mdlguy_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			
			for evt in data["events"]:
				for fmt in nonstandard_audio_formats:
					if ('.%s' % fmt) in evt['options']:
						path = 'sound/%s' % evt["options"]
						had_nonstandard_audio = True
						
						if os.path.exists(path):
							convert_audio(evt["options"], 'wav', 22050)
						elif not os.path.exists(os.path.splitext(path)[0] + ".wav"):
							print("Missing model audio: %s" % path)
						break
						
			if had_nonstandard_audio:
				mdlguy_command = [modelguy_path, 'wavify', mdl]
				print(' '.join(mdlguy_command))
				subprocess.run(mdlguy_command)
		os.remove(json_path)
	
	print ("Converting skies")
	for sky in all_skies:
		with Image.open(sky) as image:
			width, height = image.size
			
			if width > 256:
				x = 256
				y = 256
				print("Resizing invalid sky texture: %s (%dx%d -> %dx%d)" % (sky, width, height, x, y))
				image = image.resize((x, y), Image.ANTIALIAS)
				image.save(sky)

print("\nSearching for incompatible entity settings...")

gsr_files = []

last_progress_str = ''
for idx, map_name in enumerate(all_maps):
	map_path = os.path.join(maps_dir, map_name)

	progress_str = "Progress: %s / %s  (%s)" % (idx+1, len(all_maps), map_name)
	padded_progress_str = progress_str
	if len(progress_str) < len(last_progress_str):
		padded_progress_str += ' '*(len(last_progress_str) - len(progress_str))
	last_progress_str = progress_str
	print(padded_progress_str, end='\r')
	
	all_ents = load_entities(map_path)
	old_ents = copy.deepcopy(all_ents)
	
	special_map_logic = False
	
	for ent in all_ents:
		if ent.get('classname', '') == 'worldspawn' and ent.get('globalsoundlist', ''):
			# keep classname last
			path = os.path.normpath("sound/%s/%s" % (map_name.replace(".bsp", ""), ent['globalsoundlist']))
			gsr_files.append(path)
		if ent.get('soundlist', ''):
			# keep classname last
			path = os.path.normpath("sound/%s/%s" % (map_name.replace(".bsp", ""), ent['soundlist']))
			gsr_files.append(path)
	
	if check_map_problems(all_ents, False) or special_map_logic:
		print()
		if not fix_problems:
			continue
		
		# fix the problems
		check_map_problems(all_ents, True)
		
		# map-specific conversion logic
		if special_map_logic:
			convert_ents(all_ents)
		
		if not ents_match(all_ents, old_ents):
			# ripent the map
			ent_file = map_path.replace(".bsp", ".ent")
			old_ent_file = ent_file.replace(".ent", "_old.ent")
			
			# export current ents for diff
			ripent_command = [ripent_path, '-export', map_path]
			print(' '.join(ripent_command))
			subprocess.run(ripent_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			if os.path.exists(old_ent_file):
				os.remove(old_ent_file)
			os.rename(ent_file, ent_file.replace(".ent", "_old.ent"))
			
			with open(ent_file, 'w', newline='\n', encoding='utf-8',) as file:
				for ent in all_ents:
					if ent.get('classname', ''):
						# keep classname last
						cname = ent['classname']
						del ent['classname']
						ent['classname'] = cname
				
					file.write('{\n')
					
					for key,value in ent.items():
						file.write('"%s" "%s"\n' % (key, value))
					
					file.write('}\n')
					
			ripent_command = [ripent_path, '-import', map_path]
			print(' '.join(ripent_command))
			subprocess.run(ripent_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			
			os.remove(ent_file)
			os.remove(old_ent_file)
		
		print()

for cfg in all_cfgs:
	if cfg.endswith("_skl.cfg"):
		convert_skill_cfg("maps/%s" % cfg)
		continue
	with open("maps/%s" % cfg, 'r') as file:
		for line in file:
			line = line.strip().lower()
			parts = line.split()
			if len(parts) < 2:
				continue
			
			if parts[0] == 'globalsoundlist':
				mapname = os.path.splitext(cfg)[0]
				path = os.path.normpath("sound/%s/%s" % (mapname, parts[1]))
				gsr_files.append(path)

if len(gsr_files):
	print("Converting GSR audio")

for gsr in gsr_files:
	if not os.path.exists(gsr):
		print("Missing GSR: %s" % gsr)
		continue
	
	new_lines = []
	any_replaced = False
	with open(gsr, 'r') as file:
		for line in file:
			line = line.strip()
			parts = line.split()
			
			if len(parts) < 2:
				new_lines.append(line)
				continue
			
			needsReplace = False
			for fmt in nonstandard_audio_formats:
				if (".%s" % fmt) in parts[1]:
					needsReplace = True
			
			if needsReplace:
				oldpath = parts[1].replace('"', '')
				newpath = '"' + os.path.splitext(parts[1])[0].replace('"', '') + '.wav"'
				
				convert_audio(oldpath, 'wav', 22050)
				
				line = '%s %s' % (parts[0], newpath)
				new_lines.append(line)
				any_replaced = True
	
	if any_replaced:
		with open(gsr, 'w') as file:
			for line in new_lines:
				file.write(line + "\n")

# convert whatever wavs are left. Some may be referenced in scripts and so weren't detected earlier
all_wavs = get_all_wavs('sound')
for wav in all_wavs:
	needs_convert, samprate = wav_needs_conversion(wav, False)
	if needs_convert:
		print()
		file = wav[len('sound/'):]
		convert_audio(file, 'wav', samprate)

print()
os.system('pause')