#include "BaseVSShader.h"

#include "Finalblend_ps20.inc"

BEGIN_VS_SHADER( Blend, "Help for the blend shader" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FBTEXTURE,   SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( BLURTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
	END_SHADER_PARAMS

	SHADER_INIT
	{ 
		if( params[FBTEXTURE]->IsDefined() )
		{
			LoadTexture( FBTEXTURE );
		}
		if( params[BLURTEXTURE]->IsDefined() )
		{
			LoadTexture( BLURTEXTURE );
		}
		if( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE );
		}
	}
	
	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
			return "Wireframe";
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );

			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			
			pShaderShadow->SetPixelShader( "finalblend_ps20", 0 ); 
			pShaderShadow->SetVertexShader( "finalblend_vs20", 0 );

			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA);
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, FBTEXTURE, -1 );
			BindTexture( SHADER_TEXTURE_STAGE1, BLURTEXTURE, -1 );
			BindTexture( SHADER_TEXTURE_STAGE2, BASETEXTURE, -1);

		}
		Draw();
	}
END_SHADER
