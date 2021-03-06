// TextureManager.cpp: implementation of the CResourceManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable:4995)
#include <d3dx9.h>
#pragma warning(default:4995)

#include "ResourceManager.h"
#include "tss.h"
#include "blenders\blender.h"
#include "blenders\blender_recorder.h"
#include "../../xrCPU_Pipe/ttapi.h"

//	Already defined in Texture.cpp
void fix_texture_name(LPSTR fn);
//--------------------------------------------------------------------------------------------------------------
template <class T>
BOOL	reclaim		(xr_vector<T*>& vec, const T* ptr)
{
	xr_vector<T*>::iterator it	= vec.begin	();
	xr_vector<T*>::iterator end	= vec.end	();
	for (; it!=end; it++)
		if (*it == ptr)	{ vec.erase	(it); return TRUE; }
		return FALSE;
}

//--------------------------------------------------------------------------------------------------------------
IBlender* CResourceManager::_GetBlender		(LPCSTR Name)
{
	R_ASSERT(Name && Name[0]);

	LPSTR N = LPSTR(Name);
	map_Blender::iterator I = m_blenders.find	(N);

//	TODO: DX10: When all shaders are ready switch to common path
#if defined(USE_DX10) || defined(USE_DX11)
	if (I==m_blenders.end())	
	{
		Msg("DX10: Shader '%s' not found in library.",Name); 
		return 0;
	}
#endif
	if (I==m_blenders.end())	{ Debug.fatal(DEBUG_INFO,"Shader '%s' not found in library.",Name); return 0; }
	else					return I->second;
}

IBlender* CResourceManager::_FindBlender		(LPCSTR Name)
{
	if (!(Name && Name[0])) return 0;

	LPSTR N = LPSTR(Name);
	map_Blender::iterator I = m_blenders.find	(N);
	if (I==m_blenders.end())	return 0;
	else						return I->second;
}

void	CResourceManager::ED_UpdateBlender	(LPCSTR Name, IBlender* data)
{
	LPSTR N = LPSTR(Name);
	map_Blender::iterator I = m_blenders.find	(N);
	if (I!=m_blenders.end())	{
		R_ASSERT	(data->getDescription().CLS == I->second->getDescription().CLS);
		xr_delete	(I->second);
		I->second	= data;
	} else {
		m_blenders.insert	(std::make_pair(xr_strdup(Name),data));
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void	CResourceManager::_ParseList(sh_list& dest, LPCSTR names)
{
	if (!names || !names[0])
 		names 	= "$null";

	dest.clear(); // intorr: To avoid memory corruption with debug runtime.
	char*	P			= (char*) names;
	svector<char,128>	N;

	while (*P)
	{
		if (*P == ',') {
			// flush
			N.push_back	(0);
			strlwr		(N.begin());

			fix_texture_name( N.begin() );
			dest.push_back(N.begin());
			N.clear		();
		} else {
			N.push_back	(*P);
		}
		P++;
	}
	if (N.size())
	{
		// flush
		N.push_back	(0);
		strlwr		(N.begin());

		fix_texture_name( N.begin() );
		dest.push_back(N.begin());
	}
}

ShaderElement* CResourceManager::_CreateElement			(ShaderElement& S)
{
	if (S.passes.empty())		return	0;

	// Search equal in shaders array
	for (u32 it=0; it<v_elements.size(); it++)
		if (S.equal(*(v_elements[it])))	return v_elements[it];

	// Create _new_ entry
	ShaderElement*	N		=	xr_new<ShaderElement>(S);
	N->dwFlags				|=	xr_resource_flagged::RF_REGISTERED;
	v_elements.push_back	(N);
	return N;
}

void CResourceManager::_DeleteElement(const ShaderElement* S)
{
	if (0==(S->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(v_elements,S))						return;
	Msg	("! ERROR: Failed to find compiled 'shader-element'");
}

Shader*	CResourceManager::_cpp_Create	(IBlender* B, LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
	CBlender_Compile	C;
	Shader				S;

	// Access to template
	C.BT				= B;
	C.bEditor			= FALSE;
	C.bDetail			= FALSE;
#ifdef _EDITOR
	if (!C.BT)			{ ELog.Msg(mtError,"Can't find shader '%s'",s_shader); return 0; }
	C.bEditor			= TRUE;
#endif

	// Parse names
	_ParseList			(C.L_textures,	s_textures	);
	_ParseList			(C.L_constants,	s_constants	);
	_ParseList			(C.L_matrices,	s_matrices	);

	// Compile element	(LOD0 - HQ)
	{
		C.iElement			= 0;
		C.bDetail			= m_textures_description.GetDetailTexture(C.L_textures[0],C.detail_texture,C.detail_scaler);
		ShaderElement		E;
		C._cpp_Compile		(&E);
		S.E[0]				= _CreateElement	(E);
	}

	// Compile element	(LOD1)
	{
		C.iElement			= 1;
		C.bDetail			= m_textures_description.GetDetailTexture(C.L_textures[0],C.detail_texture,C.detail_scaler);
		ShaderElement		E;
		C._cpp_Compile		(&E);
		S.E[1]				= _CreateElement	(E);
	}

	// Compile element
	{
		C.iElement			= 2;
		C.bDetail			= FALSE;
		ShaderElement		E;
		C._cpp_Compile		(&E);
		S.E[2]				= _CreateElement	(E);
	}

	// Compile element
	{
		C.iElement			= 3;
		C.bDetail			= FALSE;
		ShaderElement		E;
		C._cpp_Compile		(&E);
		S.E[3]				= _CreateElement	(E);
	}

	// Compile element
	{
		C.iElement			= 4;
		C.bDetail			= TRUE;	//.$$$ HACK :)
		ShaderElement		E;
		C._cpp_Compile		(&E);
		S.E[4]				= _CreateElement	(E);
	}

	// Compile element
	{
		C.iElement			= 5;
		C.bDetail			= FALSE;
		ShaderElement		E;
		C._cpp_Compile		(&E);
		S.E[5]				= _CreateElement	(E);
	}

	// Search equal in shaders array
	for (u32 it=0; it<v_shaders.size(); it++)
		if (S.equal(v_shaders[it]))	return v_shaders[it];

	// Create _new_ entry
	Shader*		N			=	xr_new<Shader>(S);
	N->dwFlags				|=	xr_resource_flagged::RF_REGISTERED;
	v_shaders.push_back		(N);
	return N;
}

Shader*	CResourceManager::_cpp_Create	(LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
#if defined(USE_DX10) || defined(USE_DX11)
		IBlender	*pBlender = _GetBlender(s_shader?s_shader:"null");
		if (!pBlender) return NULL;
		return	_cpp_Create(pBlender ,s_shader,s_textures,s_constants,s_matrices);
#else	//	USE_DX10
		return	_cpp_Create(_GetBlender(s_shader?s_shader:"null"),s_shader,s_textures,s_constants,s_matrices);
#endif	//	USE_DX10
}

Shader*		CResourceManager::Create	(IBlender*	B,		LPCSTR s_shader,	LPCSTR s_textures,	LPCSTR s_constants, LPCSTR s_matrices)
{
    return	_cpp_Create(B, s_shader, s_textures, s_constants, s_matrices);
}

Shader*		CResourceManager::Create	(LPCSTR s_shader,	LPCSTR s_textures,	LPCSTR s_constants,	LPCSTR s_matrices)
{
#if defined(USE_DX10) || defined(USE_DX11)
	if	(_lua_HasShader(s_shader))		
		return	_lua_Create	(s_shader,s_textures);
	else								
	{
		Shader* pShader = _cpp_Create	(s_shader,s_textures,s_constants,s_matrices);
		if (pShader)
			return pShader;
		else
		{
			if (_lua_HasShader("stub_default"))
				return	_lua_Create	("stub_default",s_textures);
			else
			{
				FATAL("Can't find stub_default.s");
				return 0;
			}
		}
	}
#else	//	USE_DX10
	if(_lua_HasShader(s_shader)) return	_lua_Create(s_shader,s_textures);
	else return	_cpp_Create(s_shader,s_textures,s_constants,s_matrices);
#endif	//	USE_DX10
}

void CResourceManager::Delete(const Shader* S)
{
	if (!(S->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(v_shaders,S))						return;
	Msg	("! ERROR: Failed to find complete shader");
}

void TextureLoading(LPVOID texture)
{
	CTexture* tex = (CTexture*)texture;
	tex->Load();
}

void CResourceManager::DeferredUpload()
{
	if (RDEVICE.b_is_Ready)
	{
		Msg("CResourceManager::DeferredUpload -> START, size = %d", m_textures.size());
	
		CTimer timer;
		timer.Start();
	
		if (m_textures.size() <= 100)
		{
			Msg("CResourceManager::DeferredUpload -> one thread");
			for (auto t = m_textures.begin(); t != m_textures.end(); t++)
				t->second->Load();
		}
		else
		{
			for (auto& tex : m_textures)
			{
				ttapi_AddTask(TextureLoading, tex.second);
			}
	
			ttapi_RunAllWorkers();
		}
	
		Msg("texture loading time: %d", timer.GetElapsed_ms());
	}
}

void CResourceManager::_GetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps)
{
	m_base=c_base=m_lmaps=c_lmaps=0;

	map_Texture::iterator I = m_textures.begin	();
	map_Texture::iterator E = m_textures.end	();
	for (; I!=E; I++)
	{
		u32 m = I->second->flags.MemoryUsage;
		if (strstr(I->first,"lmap"))
		{
			c_lmaps	++;
			m_lmaps	+= m;
		} else {
			c_base	++;
			m_base	+= m;
		}
	}
}
void CResourceManager::_DumpMemoryUsage()
{
	xr_multimap<u32,std::pair<u32,shared_str>> mtex;

	// sort
	{
		for (auto it : m_textures)
		{
			u32			m = it.second->flags.MemoryUsage;
			shared_str	n = it.second->cName;
			mtex.insert (std::make_pair(m, std::make_pair(it.second->dwReference,n) ));
		}
	}

	// dump
	{
		for (auto it : mtex)
			Msg("* %4.1f : [%4d] %s",float(it.first)/1024.f, it.second.first, it.second.second.c_str());
	}
}

void	CResourceManager::Evict()
{
	//	TODO: DX10: check if we really need this method
#if !defined(USE_DX10) && !defined(USE_DX11)
	CHK_DX	(HW.pDevice->EvictManagedResources());
#endif	//	USE_DX10
}
