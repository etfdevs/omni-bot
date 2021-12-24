////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// Title: OF Message Structure Definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __OF_MESSAGES_H__
#define __OF_MESSAGES_H__

#include "TF_Messages.h"

#pragma pack(push)
#pragma pack(4)

struct ETF_CvarSet
{
	char *		m_Cvar;
	char *		m_Value;
};

struct ETF_CvarGet
{
	char *		m_Cvar;
	int			m_Value;
};

#pragma pack(pop)

#endif
