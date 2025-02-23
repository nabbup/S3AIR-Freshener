/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/MainMenu.h"
#include "sonic3air/menu/GameApp.h"
#include "sonic3air/menu/MenuBackground.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/audio/AudioOut.h"
#include "sonic3air/data/SharedDatabase.h"
#include "sonic3air/ConfigurationImpl.h"
#include "sonic3air/Game.h"
#include "sonic3air/version.inc"

#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/gameview/GameView.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/helper/Utils.h"

#include "oxygen/application/video/VideoOut.h"
#include "sonic3air/helper/DiscordIntegration.h"


namespace mainmenu
{
	bool isHexDigit(char ch)
	{
		return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
	}

	enum Entry
	{
		NORMAL_GAME	= 0x01,
		ACT_SELECT	= 0x02,
		TIME_ATTACK	= 0x03,
		OPTIONS		= 0x10,
		EXTRAS		= 0x11,
		MODS		= 0x12,
		EXIT		= 0xff
	};
	MainMenu::musicId mainMenu;
}
using namespace mainmenu;


MainMenu::MainMenu(MenuBackground& menuBackground) :
	mMenuBackground(&menuBackground)
{
	// Build up menu structure
	{
		mMenuEntries.reserve(6);
		mMenuEntries.addEntry("NORMAL GAME", mainmenu::NORMAL_GAME);
		mMenuEntries.addEntry("ACT SELECT",  mainmenu::ACT_SELECT);
		mMenuEntries.addEntry("TIME ATTACK", mainmenu::TIME_ATTACK);
		mMenuEntries.addEntry("OPTIONS",	 mainmenu::OPTIONS);
		mMenuEntries.addEntry("EXTRAS",		 mainmenu::EXTRAS);
		mMenuEntries.addEntry("MODS",		 mainmenu::MODS);

	#if !defined(PLATFORM_ANDROID) && !defined(PLATFORM_IOS)
		mMenuEntries.addEntry("EXIT",		 mainmenu::EXIT);
	#endif
	}

	// Set defaults
	mMenuEntries.mSelectedEntryIndex = 0;
}

MainMenu::~MainMenu()
{
}

GameMenuBase::BaseState MainMenu::getBaseState() const
{
	switch (mState)
	{
		case State::APPEAR:				 return BaseState::FADE_IN;
		case State::SHOW:				 return BaseState::SHOW;
		case State::FADE_TO_TITLESCREEN: return BaseState::FADE_OUT;
		case State::FADE_TO_DATASELECT:	 return BaseState::FADE_OUT;
		case State::FADE_TO_SUBMENU:	 return BaseState::FADE_OUT;
		default:						 return BaseState::INACTIVE;
	}
}

void MainMenu::setBaseState(BaseState baseState)
{
	switch (baseState)
	{
		case BaseState::INACTIVE: mState = State::INACTIVE;  break;
		case BaseState::FADE_IN:  mState = State::APPEAR;  break;
		case BaseState::SHOW:	  mState = State::SHOW;  break;
		case BaseState::FADE_OUT: mState = State::FADE_TO_EXIT;  break;
	}
}

void MainMenu::onFadeIn()
{
	DiscordIntegration::resetModdedApplicationId();

	mState = State::APPEAR;

	mMenuBackground->showPreview(false);
	mMenuBackground->startTransition(MenuBackground::Target::SPLIT);

	AudioOut::instance().stopSoundContext(AudioOut::CONTEXT_INGAME + AudioOut::CONTEXT_MUSIC);

	uint64 keyString = 0x2F;
	if (!mainMenu.mMainMenuMusic.empty())
	{
		if (mainMenu.mMainMenuMusic.length() == 2 && isHexDigit(mainMenu.mMainMenuMusic[0]) && isHexDigit(mainMenu.mMainMenuMusic[1]))
		{
			keyString = rmx::parseInteger(String("0x") + mainMenu.mMainMenuMusic);
		}
		else
		{
			keyString = rmx::getMurmur2_64(mainMenu.mMainMenuMusic);
		}
	}
	AudioOut::instance().setMenuMusic(keyString);

	// Check for unlocked secrets (needed when new game versions added secrets or reduced requirements)
	Game::instance().checkForUnlockedSecrets();
}

bool MainMenu::canBeRemoved()
{
	return (mState == State::INACTIVE && mVisibility <= 0.0f);
}

void MainMenu::initialize()
{
}

void MainMenu::deinitialize()
{
}

void MainMenu::keyboard(const rmx::KeyboardEvent& ev)
{
}

void MainMenu::update(float timeElapsed)
{
	if (mState == State::APPEAR && timeElapsed > 0.0f)
	{
		uint64 keyString = 0x2F;
		if (!mainMenu.mMainMenuMusic.empty())
		{
			if (mainMenu.mMainMenuMusic.length() == 2 && isHexDigit(mainMenu.mMainMenuMusic[0]) && isHexDigit(mainMenu.mMainMenuMusic[1]))
			{
				keyString = rmx::parseInteger(String("0x") + mainMenu.mMainMenuMusic);
			}
			else
			{
				keyString = rmx::getMurmur2_64(mainMenu.mMainMenuMusic);
			}
		}
		AudioOut::instance().setMenuMusic(keyString);
	}

	GameMenuBase::update(timeElapsed);
}

void MainMenu::render()
{
	GuiBase::render();

	Drawer& drawer = EngineMain::instance().getDrawer();

	const int width = VideoOut::instance().getScreenWidth();

	const int positionY[] = { 31, 58, 85,  125, 146, 167,  195 };
	for (size_t line = 0; line < mMenuEntries.size(); ++line)
	{
		bool isMainEntry = (line < 3);

		const auto& entry = mMenuEntries[line];
		const std::string& text = entry.mOptions.empty() ? entry.mText : entry.mOptions[entry.mSelectedIndex].mText;

		const int mainEntryOffset = width > 400 ? width / 2 + 18 : width - 182;
		const int secondaryEntryoffset = width > 400 ? width / 2 + 32 : width - 168;

		const int lineOffset = (mState < State::SHOW) ? (int)(mMenuEntries.size() - 1 - line) : (int)line;
		const int px = (isMainEntry ? mainEntryOffset : secondaryEntryoffset) + roundToInt(saturate(1.0f - mVisibility - lineOffset * 0.1f) * 200.0f - mMenuEntries[line].mAnimation.mVisibility * 10.0f);
		const int py = positionY[line];
		const bool isSelected = ((int)line == mMenuEntries.mSelectedEntryIndex);
		const bool isSelectable = entry.isFullyInteractable();
		const Color colorBG = isSelectable ? (isSelected ? Color(1.0f, 1.0f, 0.5f, mVisibility) : Color(0.5f, 0.75f, 1.0f, mVisibility)) : Color(0.4f, 0.4f, 0.4f, mVisibility * 0.25f);
		const Color color   = isSelectable ? (isSelected ? Color(1.0f, 1.0f, 0.0f, mVisibility) : Color(1.0f, 1.0f, 1.0f, mVisibility)) : Color(0.4f, 0.4f, 0.4f, mVisibility * 0.75f);

		const int underlineOffset = isMainEntry ? 15 : 9;
		drawer.drawRect(Recti(px-6, py+underlineOffset+1, 250, 3), Color(0.0f, 0.0f, 0.0f, colorBG.a * mVisibility));	// Squared alpha for the shadow is intentional
		drawer.drawRect(Recti(px-7, py+underlineOffset, 250, 3), colorBG);

		if (isMainEntry)
		{
			drawer.printText(global::mSonicFontC, Recti(px, py, 200, 18), text, 4, color);
		}
		else
		{
			DrawerPrintOptions printOptions;
			printOptions.mAlignment = 4;
			printOptions.mTintColor = color;
			printOptions.mSpacing = 1;
			drawer.printText(global::mOxyfontRegular, Recti(px, py, 200, 10), text, printOptions);
		}
	}

	// Version info
	{
		// Make place for FPS display if it's shown
		int px = (int)(mRect.width - ((Configuration::instance().mPerformanceDisplay != 0) ? 32 : 2) + (1.0f - mVisibility) * 100.0f);
		std::wstring text = L"\x02c5" BUILD_STRING L" " BUILD_VARIANT;
		drawer.printText(global::mSmallfont, Vec2i(px, 1), text, 3, Color(0.2f, 0.2f, 0.2f, mVisibility * 0.3f));

		// Show whether dev mode is active, or one of the non-default glitch fix settings
		px -= 6 + global::mSmallfont.getWidth(text);
		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			drawer.printText(global::mSmallfont, Vec2i(px, 1), "DEV MODE", 3, Color(0.6f, 0.2f, 0.2f, mVisibility * 0.3f));
		}
		else
		{
			const uint32 value = ConfigurationImpl::instance().mActiveGameSettings->getValue((uint32)SharedDatabase::Setting::SETTING_FIX_GLITCHES);
			if (value < 2)
			{
				const char* txt = (value == 0) ? "NO GLITCH FIXES" : "ONLY BASIC FIXES";
				drawer.printText(global::mSmallfont, Vec2i(px, 1), txt, 3, Color(0.6f, 0.2f, 0.2f, mVisibility * 0.3f));
			}
		}
	}

	drawer.performRendering();
}

void MainMenu::triggerStartNormalGame()
{
	playMenuSound(0x63);
	mState = State::FADE_TO_DATASELECT;
	GameApp::instance().getGameView().startFadingOut();
}

void MainMenu::triggerStartActSelect()
{
	playMenuSound(0x63);
	mState = State::FADE_TO_ACTSELECT;
	GameApp::instance().getGameView().startFadingOut();
}

void MainMenu::triggerStartExtras()
{
	playMenuSound(0x63);
	mState = State::FADE_TO_EXTRAS;
	GameApp::instance().getGameView().startFadingOut();
}

void MainMenu::startNormalGame()
{
	// Init simulation
	Game::instance().startIntoDataSelect();
	GameApp::instance().onStartGame();
	mMenuBackground->setGameStartedMenu();
}

void MainMenu::openActSelectMenu()
{
	// Init simulation
	Game::instance().startIntoActSelect();
	GameApp::instance().onStartGame();
	mMenuBackground->setGameStartedMenu();
}

void MainMenu::openTimeAttack()
{
	playMenuSound(0x63);
	mMenuBackground->openTimeAttackMenu();
	mState = State::FADE_TO_SUBMENU;
}

void MainMenu::openOptions()
{
	playMenuSound(0x63);
	mMenuBackground->openOptions();
	mState = State::FADE_TO_SUBMENU;
}

void MainMenu::openExtras()
{
	// Init simulation
	Game::instance().startIntoExtras();
	GameApp::instance().onStartGame();
	mMenuBackground->setGameStartedMenu();
	//playMenuSound(0x63);
	//mMenuBackground->openExtras();
	//mState = State::FADE_TO_SUBMENU;
}

void MainMenu::openMods()
{
	playMenuSound(0x63);
	mMenuBackground->openMods();
	mState = State::FADE_TO_SUBMENU;
}

void MainMenu::exitGame()
{
	AudioOut::instance().fadeOutChannel(0, 0.25f);
	GameApp::instance().getGameView().startFadingOut(0.25f);
	mMenuBackground->startTransition(MenuBackground::Target::TITLE);
	mState = State::FADE_TO_EXIT;
}

void MainMenu::setMainMenuMusic(std::string_view sfxId)
{
	mainMenu.mMainMenuMusic = sfxId;
}