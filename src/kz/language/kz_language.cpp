#include "kz_language.h"
#include "utils/utils.h"
#include "utils/simplecmds.h"
#include "KeyValues.h"
#include "interfaces/interfaces.h"
#include "filesystem.h"
#include "utils/ctimer.h"
#include "kz/checkpoint/kz_checkpoint.h"
#include "kz/timer/kz_timer.h"

#include <vendor/ClientCvarValue/public/iclientcvarvalue.h>
#include <vendor/MultiAddonManager/public/imultiaddonmanager.h>

extern IMultiAddonManager *g_pMultiAddonManager;

// HNS: Option event listener removed - language preferences are hardcoded

extern IClientCvarValue *g_pClientCvarValue;

static_global KeyValues *translationKV;
static_global KeyValues *languagesKV;
static_global KeyValues *addonsKV;

void KZLanguageService::Init()
{
	KZLanguageService::LoadConfigFiles();
	// HNS: Option service registration removed - using hardcoded language settings
}

void KZLanguageService::LoadConfigFiles()
{
	if (translationKV)
	{
		delete translationKV;
	}
	if (languagesKV)
	{
		delete languagesKV;
	}
	if (addonsKV)
	{
		delete addonsKV;
	}
	translationKV = new KeyValues("Phrases");
	translationKV->UsesEscapeSequences(true);
	languagesKV = new KeyValues("Languages");
	languagesKV->UsesEscapeSequences(true);
	addonsKV = new KeyValues("Addons");
	addonsKV->UsesEscapeSequences(true);
	KZLanguageService::LoadTranslations();
	KZLanguageService::LoadLanguages();
}

void KZLanguageService::LoadLanguages()
{
	char fullPath[1024];
	g_SMAPI->PathFormat(fullPath, sizeof(fullPath), "%s/addons/cs2kz/translations/config.txt", g_SMAPI->GetBaseDir());
	if (!languagesKV->LoadFromFile(g_pFullFileSystem, fullPath, nullptr))
	{
		META_CONPRINT("Failed to load translation config file.\n");
	}
	g_SMAPI->PathFormat(fullPath, sizeof(fullPath), "%s/addons/cs2kz/translations/menu-addons.txt", g_SMAPI->GetBaseDir());
	if (!addonsKV->LoadFromFile(g_pFullFileSystem, fullPath, nullptr))
	{
		META_CONPRINT("Failed to load addon config file.\n");
	}
}

void KZLanguageService::LoadTranslations()
{
	char buffer[1024];
	g_SMAPI->PathFormat(buffer, sizeof(buffer), "addons/cs2kz/translations/*.phrases.txt");
	FileFindHandle_t findHandle = {};
	const char *fileName = g_pFullFileSystem->FindFirst(buffer, &findHandle);
	if (fileName)
	{
		do
		{
			char fullPath[1024];
			g_SMAPI->PathFormat(fullPath, sizeof(fullPath), "%s/addons/cs2kz/translations/%s", g_SMAPI->GetBaseDir(), fileName);
			if (!translationKV->LoadFromFile(g_pFullFileSystem, fullPath, nullptr))
			{
				META_CONPRINTF("Failed to load %s\n", fileName);
			}
			fileName = g_pFullFileSystem->FindNext(findHandle);
		} while (fileName);
		g_pFullFileSystem->FindClose(findHandle);
	}
}

void KZLanguageService::OnPlayerPreferencesLoaded()
{
	// HNS: Simplified language preferences - always use default language
	// No dynamic language switching or reconnection logic needed
}

const char *KZLanguageService::GetLanguage()
{
	return KZLanguageService::clientLanguageInfos[this->player->GetSteamId64(false)].language;
}

const char *KZLanguageService::GetTranslatedFormat(const char *language, const char *phrase)
{
	if (!translationKV->FindKey(phrase))
	{
		// META_CONPRINTF("Warning: Phrase '%s' not found, returning orignal message!\n", phrase);
		return phrase;
	}
	const char *outFormat = translationKV->FindKey(phrase)->GetString(language);
	if (outFormat[0] == '\0')
	{
		if (!V_stricmp(language, "#format"))
		{
			// It is fine to have no format.
			return NULL;
		}
		// META_CONPRINTF("Warning: Phrase '%s' not found for language %s!\n", phrase, language);
		return translationKV->FindKey(phrase)->GetString(KZ_DEFAULT_LANGUAGE);
	}
	return outFormat;
}

void KZLanguageService::UpdateLanguage(u64 xuid, const char *langKey, LanguageInfo::CacheLevel cacheLevel, bool shouldReconnect)
{
	// Manual override > Loaded preference > Queried ConVar
	auto &langInfo = KZLanguageService::clientLanguageInfos[xuid];
	const char *addon = addonsKV->GetString(langKey, addonsKV->GetString(KZ_DEFAULT_LANGUAGE));
	if (langInfo.cacheLevel > cacheLevel)
	{
		return;
	}
	langInfo.cacheLevel = cacheLevel;
	if (!KZ_STREQILEN(langInfo.lastAddon, addon, sizeof(langInfo.lastAddon)))
	{
		if (KZ_STREQI(langInfo.lastAddon, KZ_WORKSHOP_ADDON_ID))
		{
			META_CONPRINTF("[KZ::Language] Adding %s for client %lli\n", addon, xuid);
		}
		else
		{
			META_CONPRINTF("[KZ::Language] Adding %s and removing %s for client %lli\n", addon, langInfo.lastAddon, xuid);
		}
		if (g_pMultiAddonManager)
		{
			g_pMultiAddonManager->RemoveClientAddon(langInfo.lastAddon, xuid);
			g_pMultiAddonManager->AddClientAddon(addon, xuid, true);
		}
		V_strncpy(langInfo.lastAddon, addon, sizeof(langInfo.lastAddon));
	}
	V_strncpy(langInfo.language, langKey, sizeof(langInfo.language));
}

void KZLanguageService::OnPlayerConnect(u64 steamID64)
{
	if (!steamID64 || KZLanguageService::clientLanguageInfos[steamID64].cacheLevel > LanguageInfo::CacheLevel::CACHE_NONE)
	{
		return;
	}
	this->UpdateLanguage(steamID64, KZ_DEFAULT_LANGUAGE, LanguageInfo::CacheLevel::CACHE_NONE, false);
	// HNS: Client language detection removed - using fixed default language
}

KZLanguageService::LanguageInfo::LanguageInfo()
{
	V_strncpy(this->language, KZ_DEFAULT_LANGUAGE, sizeof(this->language));
}

// HNS: Language switching command removed - using fixed default language only

CON_COMMAND_F(kz_reload_translations, "Reload translation configuration files", FCVAR_NONE)
{
	KZLanguageService::LoadConfigFiles();
}
