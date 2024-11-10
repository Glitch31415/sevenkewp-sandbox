#include "rehlds.h"
#include "util.h"
#include "PluginManager.h"

void rehlds_SendBigMessage_internal(int msgType, void* data, int sz, int playerindex) {
	IGameClient* dstClient = g_RehldsSvs->GetClient(playerindex - 1);
	sizebuf_t* dstDatagram = dstClient->GetDatagram();

	if (dstDatagram->cursize + sz + 3 >= dstDatagram->maxsize) {
		ALERT(at_error, "Message %s too large: %d\n", msgTypeStr(msgType), sz);
		return;
	}

	g_RehldsFuncs->MSG_WriteByte(dstDatagram, msgType);
	g_RehldsFuncs->MSG_WriteBuf(dstDatagram, sz, data);
}

void rehlds_SendBigMessage(int msgMode, int msgType, void* data, int sz, int playerindex) {
	if (!g_RehldsFuncs) {
		ALERT(at_console, "Rehlds API not initialized!\n");
		return;
	}

	if (msgMode != MSG_BROADCAST) {
		if (playerindex > 0 && playerindex <= gpGlobals->maxClients)
			rehlds_SendBigMessage_internal(msgType, data, sz, playerindex);
		return;
	}
	
	for (int i = 1; i <= gpGlobals->maxClients; i++) {
		CBasePlayer* plr = UTIL_PlayerByIndex(i);
		if (plr)
			rehlds_SendBigMessage_internal(msgType, data, sz, i);
	}
}

void SV_ParseVoiceData_hlcoop(IGameClient* cl) {
	uint8_t chReceived[4096];
	unsigned int nDataLength = g_RehldsFuncs->MSG_ReadShort();

	if (nDataLength > sizeof(chReceived)) {
		g_RehldsFuncs->DropClient(cl, FALSE, "Invalid voice data\n");
		return;
	}

	g_RehldsFuncs->MSG_ReadBuf(nDataLength, chReceived);

	if (sv_voiceenable->value == 0.0f)
		return;

	int sender = cl->GetId();
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* plr = UTIL_PlayerByIndex(i);

		if (!plr || !g_engfuncs.pfnVoice_GetClientListening(i, sender+1)) {
			continue;
		}
		
		bool pluginWantsMute = false;

		CALL_HOOKS_VOID(pfnSendVoiceData, sender + 1, i, chReceived, nDataLength, pluginWantsMute);

		if (pluginWantsMute) {
			continue;
		}

		IGameClient* dstClient = g_RehldsSvs->GetClient(i - 1);
		sizebuf_t* dstDatagram = dstClient->GetDatagram();

		int nSendLength = nDataLength;
		if (sender + 1 == i && !dstClient->GetLoopback()) // voice_loopback
			nSendLength = 0;
		
		if (dstDatagram->cursize + nSendLength + 6 < dstDatagram->maxsize) {
			g_RehldsFuncs->MSG_WriteByte(dstDatagram, SVC_VOICEDATA);
			g_RehldsFuncs->MSG_WriteByte(dstDatagram, sender);
			g_RehldsFuncs->MSG_WriteShort(dstDatagram, nSendLength);
			g_RehldsFuncs->MSG_WriteBuf(dstDatagram, nSendLength, chReceived);
		}
	}
}

void Rehlds_HandleNetCommand(IRehldsHook_HandleNetCommand* chain, IGameClient* cl, uint8_t opcode) {
	const int clc_voicedata = 8;

	if (opcode == clc_voicedata) {
		SV_ParseVoiceData_hlcoop(cl);
		return;
	}

	chain->callNext(cl, opcode);
}

void RegisterRehldsHooks() {
	if (!g_RehldsHookchains) {
		return;
	}
	g_RehldsHookchains->HandleNetCommand()->registerHook(&Rehlds_HandleNetCommand, HC_PRIORITY_DEFAULT + 1);
}

void UnregisterRehldsHooks() {
	if (!g_RehldsHookchains) {
		return;
	}
	g_RehldsHookchains->HandleNetCommand()->unregisterHook(&Rehlds_HandleNetCommand);
}