////////////////////////////////////////////////////////////////////////////////
//
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompETF.h"
#include "ETF_Game.h"
#include "ETF_Config.h"
#include "ETF_Client.h"

//#include "System.h"
//#include "RenderBuffer.h"

#include "PathPlannerWaypoint.h"
//#include "BotPathing.h"
#include "TF_BaseStates.h"
#include "NameManager.h"
#include "ScriptManager.h"

IGame *CreateGameInstance()
{
	return new ETF_Game;
}

int ETF_Game::GetVersionNum() const
{
	return ETF_VERSION_LATEST;
}

bool ETF_Game::CheckVersion(int _version)
{
	return _version == ETF_VERSION_LATEST;
}

Client *ETF_Game::CreateGameClient()
{
	return new ETF_Client;
}

const char *ETF_Game::GetDLLName() const
{
#ifdef WIN32
	return "omnibot_etf.dll";
#else
	return "omnibot_etf.so";
#endif
}

const char *ETF_Game::GetGameName() const
{
	return "ETF";
}

const char *ETF_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "etf\\";
#else
	return "etf";
#endif
}

const char *ETF_Game::GetNavSubfolder() const
{
#ifdef WIN32
	return "etf\\nav\\";
#else
	return "etf/nav";
#endif
}

const char *ETF_Game::GetScriptSubfolder() const
{
#ifdef WIN32
	return "etf\\scripts\\";
#else
	return "etf/scripts";
#endif
}

eNavigatorID ETF_Game::GetDefaultNavigator() const 
{
	//return NAVID_RECAST;
	return NAVID_WP; 
}

bool ETF_Game::ReadyForDebugWindow() const 
{ 
	return InterfaceFuncs::GetGameState() == GAME_STATE_PLAYING; 
}

const char *ETF_Game::IsDebugDrawSupported() const 
{ 
	if(InterfaceFuncs::GetCvar("dedicated")!=0)
		return "Can't draw waypoints on dedicated server.";
	//if(strcmp(g_EngineFuncs->GetModName(), "etf"))
	//	return "Only omnibot mod can draw waypoints.";

#ifdef INTERPROCESS
	bool EnableIpc = false;
	Options::GetValue("Debug Render","EnableInterProcess",EnableIpc);
	if(!EnableIpc) 
		return "Waypoints are not visible because option EnableInterProcess in file omni-bot.cfg is false.";
#endif

	if(InterfaceFuncs::GetCvar("cg_omnibotdrawing")==0) 
		return "Waypoints are not visible because cg_omnibotdrawing is \"0\".";
	return NULL;
}

bool ETF_Game::Init()
{
	SetRenderOverlayType(OVERLAY_OPENGL);

	if ( !TF_Game::Init() )
		return false;

	AiState::FollowPath::m_OldLadderStyle = false;

	AiState::SensoryMemory::SetEntityTraceOffsetCallback( ETF_Game::ETF_GetEntityClassTraceOffset );
	AiState::SensoryMemory::SetEntityAimOffsetCallback( ETF_Game::ETF_GetEntityClassAimOffset );

	// Set up ETF specific data.
	using namespace AiState;
	TF_Options::GRENADE_VELOCITY = 650.f;
	TF_Options::PIPE_MAX_DEPLOYED = 6;

	//TF_Options::DisguiseTeamFlags[ ETF_TEAM_BLUE ] = TF_PWR_DISGUISE_BLUE;
	//TF_Options::DisguiseTeamFlags[ ETF_TEAM_RED ] = TF_PWR_DISGUISE_RED;
	//TF_Options::DisguiseTeamFlags[ ETF_TEAM_GREEN ] = TF_PWR_DISGUISE_GREEN;
	//TF_Options::DisguiseTeamFlags[ ETF_TEAM_YELLOW ] = TF_PWR_DISGUISE_YELLOW;

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/etf_autoexec.gm", threadId);	
	ScriptManager::GetInstance()->ExecuteFile("scripts/etf_autoexec_user.gm", threadId);

	return true;
}

void ETF_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 64.f;
}

static const IntEnum ETF_TeamEnum [] =
{
	IntEnum( "SPECTATOR", OB_TEAM_SPECTATOR ),
	IntEnum( "NONE", ETF_TEAM_NONE ),
	IntEnum( "RED", ETF_TEAM_RED ),
	IntEnum( "BLUE", ETF_TEAM_BLUE ),
	IntEnum( "YELLOW", ETF_TEAM_YELLOW ),
	IntEnum( "GREEN", ETF_TEAM_GREEN ),
};

void ETF_Game::GetTeamEnumeration( const IntEnum *&_ptr, int &num )
{
	num = sizeof( ETF_TeamEnum ) / sizeof( ETF_TeamEnum[ 0 ] );
	_ptr = ETF_TeamEnum;
}

IntEnum g_ETFClassMappings[] =
{
	IntEnum("CORPSE",			ETF_CLASSEX_CORPSE),
	IntEnum("FLASH_GRENADE",	ETF_CLASSEX_FLASH_GRENADE),
};

const char *ETF_Game::FindClassName(obint32 _classId)
{
	obint32 iNumMappings = sizeof(g_ETFClassMappings) / sizeof(g_ETFClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		if(g_ETFClassMappings[i].m_Value == _classId)
			return g_ETFClassMappings[i].m_Key;
	}
	return TF_Game::FindClassName(_classId);
}

void ETF_Game::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	TF_Game::InitScriptClasses(_machine, _table);

	FilterSensory::ANYPLAYERCLASS = TF_CLASS_ANY;

	obint32 iNumMappings = sizeof(g_ETFClassMappings) / sizeof(g_ETFClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		_table->Set(_machine, g_ETFClassMappings[i].m_Key, gmVariable(g_ETFClassMappings[i].m_Value));
	}
}

void ETF_Game::InitScriptEntityFlags( gmMachine *_machine, gmTableObject *_table )
{
	//Override TF_Game, because we don't want caltrops, sabotage or radiotagged but we do want blind flag
	IGame::InitScriptEntityFlags( _machine, _table );

	_table->Set( _machine, "NEED_HEALTH", gmVariable( TF_ENT_FLAG_SAVEME ) );
	_table->Set( _machine, "NEED_ARMOR", gmVariable( TF_ENT_FLAG_ARMORME ) );
	_table->Set( _machine, "BURNING", gmVariable( TF_ENT_FLAG_BURNING ) );
	_table->Set( _machine, "TRANQUED", gmVariable( TF_ENT_FLAG_TRANQED ) );
	_table->Set( _machine, "INFECTED", gmVariable( TF_ENT_FLAG_INFECTED ) );
	_table->Set( _machine, "GASSED", gmVariable( TF_ENT_FLAG_GASSED ) );
	_table->Set( _machine, "SNIPE_AIMING", gmVariable( ENT_FLAG_IRONSIGHT ) );
	_table->Set( _machine, "AC_FIRING", gmVariable( TF_ENT_FLAG_ASSAULTFIRING ) );
	_table->Set( _machine, "LEGSHOT", gmVariable( TF_ENT_FLAG_LEGSHOT ) );
	_table->Set( _machine, "BLIND", gmVariable( ETF_ENT_FLAG_BLIND ) );
	_table->Set( _machine, "BUILDING_SG", gmVariable( TF_ENT_FLAG_BUILDING_SG ) );
	_table->Set( _machine, "BUILDING_DISP", gmVariable( TF_ENT_FLAG_BUILDING_DISP ) );
	_table->Set( _machine, "BUILDING_DETP", gmVariable( TF_ENT_FLAG_BUILDING_DETP ) );
	_table->Set( _machine, "BUILDINPROGRESS", gmVariable( TF_ENT_FLAG_BUILDINPROGRESS ) );
	_table->Set( _machine, "LEVEL2", gmVariable( TF_ENT_FLAG_LEVEL2 ) );
	_table->Set( _machine, "LEVEL3", gmVariable( TF_ENT_FLAG_LEVEL3 ) );
	_table->Set( _machine, "CONCUSSED", gmVariable( ETF_ENT_FLAG_CONCED ) );
	_table->Set( _machine, "DISGUISED", gmVariable( ETF_ENT_FLAG_DISGUISED ) );
}

void ETF_Game::InitScriptPowerups( gmMachine *_machine, gmTableObject *_table )
{
	TF_Game::InitScriptPowerups( _machine, _table );

	_table->Set( _machine, "QUAD", gmVariable( ETF_PWR_QUAD ) );
	_table->Set( _machine, "SUIT", gmVariable( ETF_PWR_SUIT ) );
	_table->Set( _machine, "HASTE", gmVariable( ETF_PWR_HASTE ) );
	_table->Set( _machine, "INVIS", gmVariable( ETF_PWR_INVIS ) );
	_table->Set( _machine, "REGEN", gmVariable( ETF_PWR_REGEN ) );
	_table->Set( _machine, "FLIGHT", gmVariable( ETF_PWR_FLIGHT ) );
	_table->Set( _machine, "INVULN", gmVariable( ETF_PWR_INVULN ) );
	_table->Set( _machine, "AQUALUNG", gmVariable( ETF_PWR_AQUALUNG ) );
	//_table->Set(_machine, "CEASEFIRE",	gmVariable(ETF_PWR_CEASEFIRE));
}

const float ETF_Game::ETF_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > TF_CLASS_NONE && _class < FilterSensory::ANYPLAYERCLASS)
	{
		if (_entflags.CheckFlag(ENT_FLAG_PRONED))
			return 16.0f;
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 24.0f;
		return 48.0f;
	}
	return TF_Game::TF_GetEntityClassTraceOffset( _class, _entflags );
}

const float ETF_Game::ETF_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > TF_CLASS_NONE && _class < FilterSensory::ANYPLAYERCLASS)
	{
		if (_entflags.CheckFlag(ENT_FLAG_PRONED))
			return 8.0f;
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 16.0f;
		return 40.0f;
	}
	return TF_Game::TF_GetEntityClassAimOffset( _class, _entflags );
}
