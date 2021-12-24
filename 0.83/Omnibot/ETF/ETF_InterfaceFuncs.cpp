////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompETF.h"
#include "ETF_InterfaceFuncs.h"

namespace InterfaceFuncs
{
	bool SetCvar(char *_cvar, char *_value)
	{
		if (_cvar && _value)
		{
			ETF_CvarSet data;
			data.m_Cvar = _cvar;
			data.m_Value = _value;
			MessageHelper msg(ETF_MSG_SETCVAR, &data, sizeof(data));
			InterfaceMsg(msg);
		}
		return true;
	}

	int GetCvar(char *_cvar)
	{
		if (_cvar)
		{
			ETF_CvarGet data;
			data.m_Cvar = _cvar;
			data.m_Value = 0;
			MessageHelper msg(ETF_MSG_GETCVAR, &data, sizeof(data));
			InterfaceMsg(msg);
			return data.m_Value;
		}
		return 0;
	}
};