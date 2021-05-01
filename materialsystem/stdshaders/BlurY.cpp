#include "BaseVSShader.h"

// These are defined to make sure the blur texture can be downsized for our render target
// which. As it stands now, it will be clamped to a height and width about 1/4 the size of 
// the backbuffer. This helps the performance (since we're performing on very few pixels) and
// the effect (since the texture is even more blurry when blown back up). 

#define WIDTH  256
#define HEIGHT 192

#define CLAMP_WIDTH( _w ) ( ( _w < WIDTH ) ? _w : WIDTH )
#define CLAMP_HEIGHT( _h ) ( ( _h < HEIGHT ) ? _h : HEIGHT )

BEGIN_VS_SHADER( BlurX, "Help for BlurY shader" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FRAMETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if( params[FRAMETEXTURE]->IsDefined() )
		{
			LoadTexture( FRAMETEXTURE );
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
			pShaderShadow->EnableAlphaWrites( true );

			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			//pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			/*
			We use the same HLSL shaders in both high-level source .cpp shaders. 
			We just feed them different constant values for the texture coordiante
			offsets.
			*/

			pShaderShadow->SetVertexShader( "blur_vs20", 0 ); 
			pShaderShadow->SetPixelShader( "blur_ps20", 0 );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, FRAMETEXTURE, -1 );
		

			int width, height;
			pShaderAPI->GetBackBufferDimensions( width, height );

			float v[4];

			float Y = 1.0f / CLAMP_HEIGHT( height / 4.0f );

			// offsets
			v[0] = 0.0f;
			v[1] = 0.10f * Y;
			pShaderAPI->SetVertexShaderConstant( 90, v, 1 );
			v[0] = 0.0f;
			v[1] = 0.75f * Y;
			pShaderAPI->SetVertexShaderConstant( 91, v, 1 );
			v[0] = 0.0f;
			v[1] = 1.10f * Y;
			pShaderAPI->SetVertexShaderConstant( 92, v, 1 );

		}
		Draw();
	}
END_SHADER
