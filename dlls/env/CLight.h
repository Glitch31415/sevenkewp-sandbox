#pragma once

class CLight : public CPointEntity
{
public:
	virtual void	KeyValue(KeyValueData* pkvd);
	virtual void	Spawn(void);
	void	Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	virtual CLight* MyLightPointer(void) { return this; }

	virtual int		Save(CSave& save);
	virtual int		Restore(CRestore& restore);

	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iStyle;	
	int		m_iszPattern;
};