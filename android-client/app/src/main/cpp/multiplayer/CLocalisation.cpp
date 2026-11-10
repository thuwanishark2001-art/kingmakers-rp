#include "CLocalisation.h"

#include "main.h"
#include "gui/gui.h"
#include "game/game.h"
#include "net/netgame.h"
#include "game/RW/RenderWare.h"
#include "chatwindow.h"
#include "playertags.h"
#include "keyboard.h"
#include "CSettings.h"

extern CGUI* pGUI;

char CLocalisation::m_szMessages[E_MSG::MSG_COUNT][MAX_LOCALISATION_LENGTH] = {
	"{bbbbbb}Connecting to Kingmakers RP...{ffffff}",
	"{bbbbbb}Connected. Preparing the game...{ffffff}",
	"{bbbbbb}The server closed the connection{ffffff}",
	"{ff0000}Some game files were modified, reinstall the client{ffffff}",
	"{bbbbbb}unused{ffffff}",
	"{ff0000}You were disconnected from the server. Restart the game{ffffff}",
	"{bbbbbb}Server is not responding. Reconnecting...{ffffff}",
	"{bbbbbb}Network problems, reconnecting...{ffffff}",
	"{bbbbbb}Server is full{ffffff}"};


void CLocalisation::Initialise()
{
    CHook::CallFunction<void>("_ZN13CLocalisation10InitialiseEv");
}

void CLocalisation::Initialise(const char* szFile)
{
	char buff[MAX_LOCALISATION_LENGTH];
	snprintf(&buff[0], sizeof(buff), "%sSAMP/%s", GetStorageDirectory(), szFile);

	FILE* pFile = fopen(&buff[0], "r");
	if (!pFile)
	{
		Log("Localisation | Cannot initialise %s", szFile);
		return;
	}

	for (int i = 0; i < E_MSG::MSG_COUNT; i++)
	{
		memset(m_szMessages[i], 0, MAX_LOCALISATION_LENGTH);
	}
	uint32_t counter = 0;
	while (fgets(&buff[0], MAX_LOCALISATION_LENGTH, pFile) != NULL)
	{
		if (counter == E_MSG::MSG_COUNT)
		{
			break;
		}

		memcpy((void*)& m_szMessages[counter][0], (const void*)(&buff[0]), MAX_LOCALISATION_LENGTH);
		counter++;
	}
	fclose(pFile);
	Log("Localisation initialized");
}

char* CLocalisation::GetMessage(E_MSG msg)
{
	//Log("SYMBOLS debug %x %x %x %x %x %x %x %x %x", m_szMessages[msg][0], m_szMessages[msg][1], m_szMessages[msg][2], m_szMessages[msg][3], m_szMessages[msg][4], m_szMessages[msg][5],
		//m_szMessages[msg][6], m_szMessages[msg][7], m_szMessages[msg][8]);
	//Log("SYMBOLS debug %s", &m_szMessages[msg][0]);
	return &m_szMessages[msg][0];
}
