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
static GLuint lightOrgin;
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

static GLuint diffuseColor;
static GLuint specularColor;

static GLuint colorModulate;
static GLuint colorAdd;

static GLuint wvp;
static GLuint normalCubeMapImage;
static GLuint bumpImage;
static GLuint lightFalloffImage;
static GLuint lightImage;
static GLuint diffuseImage;
static GLuint specularImage;
static GLuint specularTableImage;

/*
==================
RB_ARB2_DrawInteraction
==================
*/
void	RB_GLSL_DrawInteraction( const drawInteraction_t *din ) {
	// load all the vertex program parameters

	glUniform3fv(viewOrgin, 1, din->localViewOrigin.ToFloatPtr());
	glUniform4fv(lightProjS, 1, din->lightProjection[0].ToFloatPtr());
	glUniform4fv(lightProjT, 1, din->lightProjection[1].ToFloatPtr());
	glUniform4fv(lightProjQ, 1, din->lightProjection[2].ToFloatPtr());
	glUniform4fv(lightFallOffS, 1, din->lightProjection[3].ToFloatPtr());
	glUniform4fv(bumpMatrixS, 1, din->bumpMatrix[0].ToFloatPtr());
	glUniform4fv(bumpMatrixT, 1, din->bumpMatrix[1].ToFloatPtr());
	glUniform4fv(diffuseMatrixS, 1, din->diffuseMatrix[0].ToFloatPtr());
	glUniform4fv(diffuseMatrixT, 1, din->diffuseMatrix[1].ToFloatPtr());
	glUniform4fv(specularMatrixS, 1, din->specularMatrix[0].ToFloatPtr());
	glUniform4fv(specularMatrixT, 1, din->specularMatrix[1].ToFloatPtr());

	static const float zero[4] = { 0, 0, 0, 0 };
	static const float one[4] = { 1, 1, 1, 1 };
	static const float negOne[4] = { -1, -1, -1, -1 };

	switch ( din->vertexColor ) {
	case SVC_IGNORE:
		glUniform4fv(colorModulate, 1, zero);
		glUniform4fv(colorAdd, 1, one);
		break;
	case SVC_MODULATE:
		glUniform4fv(colorModulate, 1, one);
		glUniform4fv(colorAdd, 1, zero);
		break;
	case SVC_INVERSE_MODULATE:
		glUniform4fv(colorModulate, 1, negOne);
		glUniform4fv(colorAdd, 1, one);
		break;
	}

	glUniform4fv(diffuseColor, 1, din->diffuseColor.ToFloatPtr());
	glUniform4fv(specularColor, 1, din->specularColor.ToFloatPtr());

	// texture 1 will be the per-surface bump map
	glActiveTexture(GL_TEXTURE1);
	backEnd.glState.currenttmu = 1;
	din->bumpImage->Bind();

	// texture 2 will be the light falloff texture
	glActiveTexture(GL_TEXTURE2);
	backEnd.glState.currenttmu = 2;
	din->lightFalloffImage->Bind();

	// texture 3 will be the light projection texture
	glActiveTexture(GL_TEXTURE3);
	backEnd.glState.currenttmu = 3;
	din->lightImage->Bind();

	// texture 4 is the per-surface diffuse map
	glActiveTexture(GL_TEXTURE4);
	backEnd.glState.currenttmu = 4;
	din->diffuseImage->Bind();

	// texture 5 is the per-surface specular map
	glActiveTexture(GL_TEXTURE5);
	backEnd.glState.currenttmu = 5;
	din->specularImage->Bind();

	RB_DrawElementsWithCounters( din->surf->geo );
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
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc );

	//GL_Cull(CT_TWO_SIDED);
	qglEnableClientState(GL_VERTEX_ARRAY);
	//qglDisableClientState(GL_VERTEX_ARRAY);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glUseProgram(program);

	float	modelMatrix[16];
	
	myGlMultMatrix( surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, modelMatrix);
	glUniformMatrix4fv(wvp, 1, GL_FALSE, &modelMatrix[0] );
	//glUniformMatrix4fv(wvp1, 1, GL_FALSE, &surf->space->modelViewMatrix[0] );
	//const float* mat = backEnd.viewDef->projectionMatrix; 
	//glUniformMatrix4fv(wvp, 1, GL_FALSE, &mat[0] );


	// enable the vertex arrays
	glEnableVertexAttribArray( 1 );
	glEnableVertexAttribArray( 2 );
	glEnableVertexAttribArray( 3 );
	glEnableVertexAttribArray( 4 );
	glEnableVertexAttribArray( 5 );
	glEnableVertexAttribArray( 6 );

	// texture 0 is the normalization cube map for the vector towards the light
	glActiveTexture(GL_TEXTURE0);
	backEnd.glState.currenttmu = 0;
	if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		globalImages->ambientNormalMap->Bind();
	} else {
		globalImages->normalCubeMapImage->Bind();
	}

	// texture 6 is the specular lookup table
	glActiveTexture(GL_TEXTURE6);
	backEnd.glState.currenttmu = 6;
	if ( r_testARBProgram.GetBool() ) {
		globalImages->specular2DTableImage->Bind();	// variable specularity in alpha channel
	} else {
		globalImages->specularTableImage->Bind();
	}

	for ( ; surf ; surf=surf->nextOnLight ) {
		// set the vertex pointers
		idDrawVert	*ac = (idDrawVert *)vertexCache.Position( surf->geo->ambientCache );

		qglVertexPointer( 3, GL_FLOAT, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), ac->st.ToFloatPtr());
		glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(idDrawVert), ac->color);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), ac->normal.ToFloatPtr());
		glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), ac->tangents[0].ToFloatPtr());
		glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), ac->tangents[1].ToFloatPtr());

		idVec4 localLight;

		R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.vLight->globalLightOrigin, localLight.ToVec3() );
		localLight.w = 0.0f;
		glUniform3fv(lightOrgin, 1, localLight.ToFloatPtr());
		//glDrawElements(GL_TRIANGLES, surf->geo->numIndexes, GL_UNSIGNED_SHORT, surf->geo->indexes);
		// this may cause RB_ARB2_DrawInteraction to be exacuted multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions( surf, RB_GLSL_DrawInteraction );
	}

	glDisableVertexAttribArray( 1 );
	glDisableVertexAttribArray( 2 );
	glDisableVertexAttribArray( 3 );
	glDisableVertexAttribArray( 4 );
	glDisableVertexAttribArray( 5 );
	glDisableVertexAttribArray( 6 );

	// disable features
	glActiveTexture( GL_TEXTURE6 );
	backEnd.glState.currenttmu = 6;
	globalImages->BindNull();

	glActiveTexture( GL_TEXTURE5 );
	backEnd.glState.currenttmu = 5;
	globalImages->BindNull();

	glActiveTexture( GL_TEXTURE4 );
	backEnd.glState.currenttmu = 4;
	globalImages->BindNull();

	glActiveTexture( GL_TEXTURE3 );
	backEnd.glState.currenttmu = 3;
	globalImages->BindNull();

	glActiveTexture( GL_TEXTURE2 );
	backEnd.glState.currenttmu = 2;
	globalImages->BindNull();

	glActiveTexture( GL_TEXTURE1 );
	backEnd.glState.currenttmu = 1;
	globalImages->BindNull();

	glUseProgram(0);

	backEnd.glState.currenttmu = -1;
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
	glBindAttribLocation(program, 3, "vColor");
	glBindAttribLocation(program, 4, "vNormal");
	glBindAttribLocation(program, 5, "vTangent");
	glBindAttribLocation(program, 6, "vBinormal");
	GL_LinkProgram(program, vert, frag);

	wvp = glGetUniformLocation(program, "WVP");
	lightOrgin = glGetUniformLocation(program, "fvLightPosition");
	viewOrgin = glGetUniformLocation(program, "fvEyePosition");
	lightProjS = glGetUniformLocation(program, "lightProjectionS");
	lightProjT = glGetUniformLocation(program, "lightProjectionT");
	lightProjQ = glGetUniformLocation(program, "lightProjectionQ");
	lightFallOffS = glGetUniformLocation(program, "lightFallOff");
	bumpMatrixS = glGetUniformLocation(program, "bumpMatrixS");
	bumpMatrixT = glGetUniformLocation(program, "bumpMatrixT");
	diffuseMatrixS = glGetUniformLocation(program, "diffuseMatrixS");
	diffuseMatrixT = glGetUniformLocation(program, "diffuseMatrixT");
	specularMatrixS = glGetUniformLocation(program, "specularMatrixS");
	specularMatrixT = glGetUniformLocation(program, "specularMatrixT");

	normalCubeMapImage = glGetUniformLocation(program, "normalCubeMapImage");
	bumpImage = glGetUniformLocation(program, "bumpImage");
	lightFalloffImage = glGetUniformLocation(program, "lightFalloffImage");
	lightImage = glGetUniformLocation(program, "lightImage");
	diffuseImage = glGetUniformLocation(program, "diffuseImage");
	specularImage = glGetUniformLocation(program, "specularImage");
	specularTableImage = glGetUniformLocation(program, "specularTableImage");
	diffuseColor = glGetUniformLocation(program, "diffuseColor");
	specularColor = glGetUniformLocation(program, "specularColor");

	colorModulate = glGetUniformLocation(program, "colorModulate");
	colorAdd = glGetUniformLocation(program, "colorAdd");

	glUseProgram(program);
	glUniform1i(normalCubeMapImage, 0);
	glUniform1i(bumpImage, 1);
	glUniform1i(lightFalloffImage, 2);
	glUniform1i(lightImage, 3);
	glUniform1i(diffuseImage, 4);
	glUniform1i(specularImage, 5);
	glUniform1i(specularTableImage, 6);
	glUseProgram(0);
}

void RB_GLSL_DrawInteractions( void )
{
	viewLight_t		*vLight;
	const idMaterial	*lightShader;


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

		qglEnable( GL_VERTEX_PROGRAM_ARB );
		// stencil shadow shader
		qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW );
		RB_StencilShadowPass( vLight->globalShadows );
		// interaction shader
		qglDisableClientState(GL_VERTEX_ARRAY);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		RB_GLSL_CreateDrawInteractions( vLight->localInteractions );

		qglEnableClientState(GL_VERTEX_ARRAY);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglEnable( GL_VERTEX_PROGRAM_ARB );
		qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW );
		RB_StencilShadowPass( vLight->localShadows );

		qglDisableClientState(GL_VERTEX_ARRAY);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		RB_GLSL_CreateDrawInteractions( vLight->globalInteractions );
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglDisable( GL_VERTEX_PROGRAM_ARB );

		//RB_StencilShadowPass( vLight->globalShadows );
		//RB_GLSL_CreateDrawInteractions( vLight->localInteractions );
		//RB_StencilShadowPass( vLight->localShadows );
		//RB_GLSL_CreateDrawInteractions( vLight->globalInteractions );

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

