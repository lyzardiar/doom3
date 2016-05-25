/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "glutils.h"
#include "tr_local.h"

/*

There are not enough vertex program texture coordinate outputs
to have unique texture coordinates for bump, specular, and diffuse,
so diffuse and specular are assumed to be the same mapping.

To handle properly, those cases with different diffuse and specular
mapping will need to be run as two passes.

*/

static const int MAX_BUFFER_LEN = 1024; 


static GLuint program;
static GLuint viewOrgin;
static GLuint lightProjS;
static GLuint lightProjT;
static GLuint lightProjQ;
static GLuint lightFallOffS;
static GLuint bumpMatrixS;
static GLuint bumpMatrixT;
static GLuint diffuseMatrixS;
static GLuint diffuseMatrixT;
static GLuint specularMatrixS;
static GLuint specularMatrixT;

static GLuint wvp;
static GLuint texture1;

/*
==================
RB_ARB2_DrawInteraction
==================
*/
void	RB_GLSL_DrawInteraction( const drawInteraction_t *din ) {
	// load all the vertex program parameters

	//glUniform4fv(viewOrgin, 1, din->localViewOrigin.ToFloatPtr());
	//glUniform4fv(lightProjS, 1, din->lightProjection[0].ToFloatPtr());
	//glUniform4fv(lightProjT, 1, din->lightProjection[1].ToFloatPtr());
	//glUniform4fv(lightProjQ, 1, din->lightProjection[2].ToFloatPtr());
	//glUniform4fv(lightFallOffS, 1, din->lightProjection[3].ToFloatPtr());


	// texture 1 will be the per-surface bump map
	glActiveTexture(GL_TEXTURE0);
	din->bumpImage->Bind();

	// texture 2 will be the light falloff texture
	glActiveTexture(GL_TEXTURE1);
	din->lightFalloffImage->Bind();

	// texture 3 will be the light projection texture
	glActiveTexture(GL_TEXTURE2);
	din->lightImage->Bind();

	// texture 4 is the per-surface diffuse map
	glActiveTexture(GL_TEXTURE3);
	din->diffuseImage->Bind();

	// texture 5 is the per-surface specular map
	glActiveTexture(GL_TEXTURE4);
	din->specularImage->Bind();

	// draw it
	RB_DrawElementsWithCounters( din->surf->geo );
}

void R_Draw3DCoordinate()
{
	//glUseProgram(program);
	//GL_Cull(CT_TWO_SIDED);
	//qglDisableClientState(GL_VERTEX_ARRAY);
	//qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	//qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	//qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
	float modelMatrix[16];
	// -z
	// -x
	// y

	// x是-y轴 y是-z轴 z是-x轴
	// z = -y y=-x z=-x
	//	0, 0, -1, 0,
	//	-1, 0, 0, 0,
	//	0, 1, 0, 0,
	//	0, 0, 0, 1
	memset(modelMatrix, 0, 4*16);
	modelMatrix[2] = modelMatrix[4] = -1;
	modelMatrix[9] = modelMatrix[15] = 1;
	myGlMultMatrix( backEnd.viewDef->worldSpace.modelViewMatrix, backEnd.viewDef->projectionMatrix, modelMatrix);
	//memcpy(modelMatrix, backEnd.viewDef->worldSpace.modelViewMatrix, 16*4);
	glUniformMatrix4fv(wvp, 1, GL_FALSE, &modelMatrix[0] );

	// enable the vertex arrays
	glEnableVertexAttribArray( 1 );
	glEnableVertexAttribArray( 2 );

	float vertices[] = {128.f, -128.f, -128.f,
		128.f, -128.f, 128.f,
		128.f, 128.f, 1.0f,
		1.0f, 0.f, 0.f
	};

	float texcoord[] = {-1.f, -1.f, 1.f,
		-1.f, 1.f, 0.f,
	};

	float color[] = {1.f, 0.f, 0.f, 0.f,
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
	};

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, vertices);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, texcoord);

	//unsigned short indices[] = {0, 1, 0, 2, 0, 3};
	//glDrawElements(GL_LINES, 4, GL_UNSIGNED_SHORT, indices);
	unsigned short indices[] = {0, 1, 2, 2, 0, 3};
	qglDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, indices);
	GL_CheckErrors();

	glDisableVertexAttribArray( 1 );
	glDisableVertexAttribArray( 2 );

	glUseProgram(0);

	qglEnableClientState(GL_VERTEX_ARRAY);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

}

/*
=============
RB_ARB2_CreateDrawInteractions

=============
*/
void RB_GLSL_CreateDrawInteractions( const drawSurf_t *surf ) {
	if ( !surf ) {
		return;
	}

	// perform setup here that will be constant for all interactions
	//GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc );
	//GL_Cull(CT_TWO_SIDED);
	qglDisableClientState(GL_VERTEX_ARRAY);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );

	glUseProgram(program);

	qglStencilFunc( GL_ALWAYS, 128, 255 );
	float	modelMatrix[16];
	float   tmp[16];
	
	myGlMultMatrix( backEnd.viewDef->worldSpace.modelViewMatrix, backEnd.viewDef->projectionMatrix, modelMatrix);
	glUniformMatrix4fv(wvp, 1, GL_FALSE, &modelMatrix[0] );

	// enable the vertex arrays
	glEnableVertexAttribArray( 1 );
	glEnableVertexAttribArray( 2 );

//	glActiveTexture(GL_TEXTURE1);

	//	glBindTexture( GL_TEXTURE_2D, material->bumpMap->GetName() );
	// texture 0 is the normalization cube map for the vector towards the light
	if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		globalImages->ambientNormalMap->Bind();
	} else {
		globalImages->normalCubeMapImage->Bind();
	}

	// texture 6 is the specular lookup table
	if ( r_testARBProgram.GetBool() ) {
		globalImages->specular2DTableImage->Bind();	// variable specularity in alpha channel
	} else {
		globalImages->specularTableImage->Bind();
	}

	for ( ; surf ; surf=surf->nextOnLight ) {
		// perform setup here that will not change over multiple interaction passes
		backEnd.glState.currenttmu = 3;
		glUniform1i( texture1,  3);

		const idMaterial	*surfaceShader = surf->material;
		const float			*surfaceRegs = surf->shaderRegisters;

		for ( int surfaceStageNum = 0 ; surfaceStageNum < surfaceShader->GetNumStages() ; surfaceStageNum++ ) {
			const shaderStage_t	*surfaceStage = surfaceShader->GetStage( surfaceStageNum );

			surfaceStage->texture.image->Bind();

		}
		//R_Draw3DCoordinate();

		// set the vertex pointers
		idDrawVert	*ac = (idDrawVert *)vertexCache.Position( surf->geo->ambientCache );
		//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
		//glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), ac->color);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), ac->st.ToFloatPtr());

		//glDrawElements(GL_TRIANGLES, surf->geo->numIndexes, GL_UNSIGNED_SHORT, surf->geo->indexes);
		// this may cause RB_ARB2_DrawInteraction to be exacuted multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions( surf, RB_GLSL_DrawInteraction );
	}

	glDisableVertexAttribArray( 1 );
	glDisableVertexAttribArray( 2 );
	glUseProgram(0);

	GL_SelectTexture(0 );

}

void R_GLSL_Init( void )
{
	GLenum err = glewInit();  
	if (GLEW_OK != err)  
	{  
		/* Problem: glewInit failed, something is seriously wrong. */  
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));  
	}  

	glConfig.allowNEWPath = true;
	
	GLuint vert = GL_CompileShaderFromFile(GL_VERTEX_SHADER, "glprogs/test.vert");
	GLuint frag = GL_CompileShaderFromFile(GL_FRAGMENT_SHADER, "glprogs/test.frag");
	program = glCreateProgram();
	glBindAttribLocation(program, 1, "vPosition");
	glBindAttribLocation(program, 2, "vTexCoord");
	GL_LinkProgram(program, vert, frag);
	wvp = glGetUniformLocation(program, "WVP");
	texture1 = glGetUniformLocation(program, "texture1");
}

void RB_GLSL_DrawInteractions( void )
{
	viewLight_t		*vLight;
	const idMaterial	*lightShader;
	//qglColor4f(1.0, 0, 0, 1.0);
	GL_SelectTexture( 0 );

	qglDisableClientState(GL_VERTEX_ARRAY);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
	//
	// for each light, perform adding and shadowing  
	// RB_GLSL_DrawInteractions 不是每帧都会绘制
	//
	for ( vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next ) {
		backEnd.vLight = vLight;

		// do fogging later
		if ( vLight->lightShader->IsFogLight() ) {
			continue;
		}
		if ( vLight->lightShader->IsBlendLight() ) {
			continue;
		}

		if ( !vLight->localInteractions && !vLight->globalInteractions
			&& !vLight->translucentInteractions ) {
				continue;
		}

		lightShader = vLight->lightShader;

		// clear the stencil buffer if needed
		if ( vLight->globalShadows || vLight->localShadows ) {
			backEnd.currentScissor = vLight->scissorRect;
			if ( r_useScissor.GetBool() ) {
				qglScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1, 
					backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
					backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
					backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
			}
			qglClear( GL_STENCIL_BUFFER_BIT );
		} else {
			// no shadows, so no need to read or write the stencil buffer
			// we might in theory want to use GL_ALWAYS instead of disabling
			// completely, to satisfy the invarience rules
			qglStencilFunc( GL_ALWAYS, 128, 255 );
		}

		RB_StencilShadowPass( vLight->globalShadows );
		RB_GLSL_CreateDrawInteractions( vLight->localInteractions );
		RB_StencilShadowPass( vLight->localShadows );
		RB_GLSL_CreateDrawInteractions( vLight->globalInteractions );

		// translucent surfaces never get stencil shadowed
		if ( r_skipTranslucent.GetBool() ) {
			continue;
		}

		qglStencilFunc( GL_ALWAYS, 128, 255 );

		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		RB_GLSL_CreateDrawInteractions( vLight->translucentInteractions );

		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
	}

	//// disable stencil shadow test
	qglStencilFunc( GL_ALWAYS, 128, 255 );

	GL_SelectTexture( 0 );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglEnableClientState( GL_VERTEX_ARRAY );

}

