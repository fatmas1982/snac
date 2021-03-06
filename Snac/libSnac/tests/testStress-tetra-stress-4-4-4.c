/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
** Copyright (C), 2003, 
**	Steve Quenette, 110 Victoria Street, Melbourne, Victoria, 3053, Australia.
**	Californian Institute of Technology, 1200 East California Boulevard, Pasadena, California, 91125, USA.
**	University of Texas, 1 University Station, Austin, Texas, 78712, USA.
**
** Authors:
**	Stevan M. Quenette, Senior Software Engineer, VPAC. (steve@vpac.org)
**	Stevan M. Quenette, Visitor in Geophysics, Caltech.
**	Luc Lavier, Research Scientist, The University of Texas. (luc@utig.ug.utexas.edu)
**	Luc Lavier, Research Scientist, Caltech.
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 2, or (at your option) any
** later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** Role:
**	Tests StrainRate function for values of strain.
**
** $Id: testStress-tetra-stress-4-4-4.c 3243 2006-10-12 09:04:00Z SteveQuenette $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <mpi.h>
#include <StGermain/StGermain.h>
#include <StGermain/FD/FD.h>
#include "Snac/Snac.h"
#include "TestStress.h"

#include "stdio.h"
#include "stdlib.h"


struct _Node {
	struct { __Snac_Node };
};

struct _Element {
	struct { __Snac_Element };
};

int main( int argc, char* argv[] ) {
	MPI_Comm			CommWorld;
	int				rank;
	int				numProcessors;
	int				procToWatch;
	Dictionary*			dictionary;
	Snac_Context*			snacContext;
	Element_Index			elementIndex;
	double				minLengthScale;
	Tetrahedra_Index		tetraIndex;

	Dictionary_Entry_Value* nodeICs;
	Dictionary_Entry_Value* vcList;
	Dictionary_Entry_Value* varStruct;
	Dictionary_Entry_Value* eachVarList;
	Dictionary_Entry_Value* vxStruct;
	Dictionary_Entry_Value* vyStruct;
	Dictionary_Entry_Value* vzStruct;
	
	/* Initialise MPI, get world info */
	MPI_Init( &argc, &argv );
	Snac_Init( &argc, &argv );
	MPI_Comm_dup( MPI_COMM_WORLD, &CommWorld );
	MPI_Comm_size( CommWorld, &numProcessors );
	MPI_Comm_rank( CommWorld, &rank );
	if( argc >= 2 ) {
		procToWatch = atoi( argv[1] );
	}
	else {
		procToWatch = 0;
	}
	if( rank == procToWatch ) printf( "Watching rank: %i\n", rank );
	
	/* Read input */
	dictionary = Dictionary_New();
	dictionary->add( dictionary, "rank", Dictionary_Entry_Value_FromUnsignedInt( rank ) );
	dictionary->add( dictionary, "numProcessors", Dictionary_Entry_Value_FromUnsignedInt( numProcessors ) );
	dictionary->add( dictionary, "meshSizeI", Dictionary_Entry_Value_FromUnsignedInt( 4 ) );
	dictionary->add( dictionary, "meshSizeJ", Dictionary_Entry_Value_FromUnsignedInt( 4 ) );
	dictionary->add( dictionary, "meshSizeK", Dictionary_Entry_Value_FromUnsignedInt( 4 ) );
	dictionary->add( dictionary, "minX", Dictionary_Entry_Value_FromDouble( 0.0f ) );
	dictionary->add( dictionary, "minY", Dictionary_Entry_Value_FromDouble( -300.0f ) );
	dictionary->add( dictionary, "minZ", Dictionary_Entry_Value_FromDouble( 0.0f ) );
	dictionary->add( dictionary, "maxX", Dictionary_Entry_Value_FromDouble( 300.0f ) );
	dictionary->add( dictionary, "maxY", Dictionary_Entry_Value_FromDouble( 0.0f ) );
	dictionary->add( dictionary, "maxZ", Dictionary_Entry_Value_FromDouble( 300.0f ) );

	nodeICs = Dictionary_Entry_Value_NewStruct();
	vcList = Dictionary_Entry_Value_NewList();
	varStruct = Dictionary_Entry_Value_NewStruct();
	eachVarList = Dictionary_Entry_Value_NewList();
	vxStruct = Dictionary_Entry_Value_NewStruct();
	vyStruct = Dictionary_Entry_Value_NewStruct();
	vzStruct = Dictionary_Entry_Value_NewStruct();
	Dictionary_Entry_Value_AddMember( vxStruct, "name", Dictionary_Entry_Value_FromString( "vx" ) );
	Dictionary_Entry_Value_AddMember( vxStruct, "type", Dictionary_Entry_Value_FromString( "double" ) );
	Dictionary_Entry_Value_AddMember( vxStruct, "value", Dictionary_Entry_Value_FromDouble( 3.0e-10f ) );
	Dictionary_Entry_Value_AddMember( vyStruct, "name", Dictionary_Entry_Value_FromString( "vy" ) );
	Dictionary_Entry_Value_AddMember( vyStruct, "type", Dictionary_Entry_Value_FromString( "double" ) );
	Dictionary_Entry_Value_AddMember( vyStruct, "value", Dictionary_Entry_Value_FromDouble( 3.0e-10f ) );
	Dictionary_Entry_Value_AddMember( vzStruct, "name", Dictionary_Entry_Value_FromString( "vz" ) );
	Dictionary_Entry_Value_AddMember( vzStruct, "type", Dictionary_Entry_Value_FromString( "double" ) );
	Dictionary_Entry_Value_AddMember( vzStruct, "value", Dictionary_Entry_Value_FromDouble( 6.0e-10f ) );
	Dictionary_Entry_Value_AddElement( eachVarList, vxStruct );
	Dictionary_Entry_Value_AddElement( eachVarList, vyStruct );
	Dictionary_Entry_Value_AddElement( eachVarList, vzStruct );
	Dictionary_Entry_Value_AddMember( varStruct, "type", Dictionary_Entry_Value_FromString( "AllNodesVC" ) );
	Dictionary_Entry_Value_AddMember( varStruct, "variables", eachVarList );
	Dictionary_Entry_Value_AddElement( vcList, varStruct );
	Dictionary_Entry_Value_AddMember( nodeICs, "vcList", vcList );
	Dictionary_Add( dictionary, "nodeICs", nodeICs );

	/* Build the context */
	snacContext = Snac_Context_New( 0.0f, 10.0f, sizeof(Snac_Node), sizeof(Snac_Element), CommWorld, dictionary );
	
	/* Construction phase -----------------------------------------------------------------------------------------------*/
	Stg_Component_Construct( snacContext, 0 /* dummy */, &snacContext, True );
	
	/* Building phase ---------------------------------------------------------------------------------------------------*/
	Stg_Component_Build( snacContext, 0 /* dummy */, False );
	
	/* Initialisaton phase ----------------------------------------------------------------------------------------------*/
	Stg_Component_Initialise( snacContext, 0 /* dummy */, False );

	/* Work out the first element's tetrahedra values, and print them. */
	printf( "Element: 0, Coords: (%g %g %g), (%g %g %g), (%g %g %g), (%g %g %g), (%g %g %g) (%g %g %g) (%g %g %g) (%g %g %g)\n",
		Snac_Element_NodeCoord( snacContext, 0, 0 )[0],
		Snac_Element_NodeCoord( snacContext, 0, 0 )[1],
		Snac_Element_NodeCoord( snacContext, 0, 0 )[2],
		Snac_Element_NodeCoord( snacContext, 0, 1 )[0],
		Snac_Element_NodeCoord( snacContext, 0, 1 )[1],
		Snac_Element_NodeCoord( snacContext, 0, 1 )[2],
		Snac_Element_NodeCoord( snacContext, 0, 3 )[0],
		Snac_Element_NodeCoord( snacContext, 0, 3 )[1],
		Snac_Element_NodeCoord( snacContext, 0, 3 )[2],
		Snac_Element_NodeCoord( snacContext, 0, 2 )[0],
		Snac_Element_NodeCoord( snacContext, 0, 2 )[1],
		Snac_Element_NodeCoord( snacContext, 0, 2 )[2],
		Snac_Element_NodeCoord( snacContext, 0, 4 )[0],
		Snac_Element_NodeCoord( snacContext, 0, 4 )[1],
		Snac_Element_NodeCoord( snacContext, 0, 4 )[2],
		Snac_Element_NodeCoord( snacContext, 0, 5 )[0],
		Snac_Element_NodeCoord( snacContext, 0, 5 )[1],
		Snac_Element_NodeCoord( snacContext, 0, 5 )[2],
		Snac_Element_NodeCoord( snacContext, 0, 7 )[0],
		Snac_Element_NodeCoord( snacContext, 0, 7 )[1],
		Snac_Element_NodeCoord( snacContext, 0, 7 )[2],
		Snac_Element_NodeCoord( snacContext, 0, 6 )[0],
		Snac_Element_NodeCoord( snacContext, 0, 6 )[1],
		Snac_Element_NodeCoord( snacContext, 0, 6 )[2] );
	printf( "Element: 0, Vel: (%g %g %g), (%g %g %g), (%g %g %g), (%g %g %g), (%g %g %g) (%g %g %g) (%g %g %g) (%g %g %g)\n",
		Snac_Element_Node_P( snacContext, 0, 0 )->velocity[0],
		Snac_Element_Node_P( snacContext, 0, 0 )->velocity[1],
		Snac_Element_Node_P( snacContext, 0, 0 )->velocity[2],
		Snac_Element_Node_P( snacContext, 0, 1 )->velocity[0],
		Snac_Element_Node_P( snacContext, 0, 1 )->velocity[1],
		Snac_Element_Node_P( snacContext, 0, 1 )->velocity[2],
		Snac_Element_Node_P( snacContext, 0, 3 )->velocity[0],
		Snac_Element_Node_P( snacContext, 0, 3 )->velocity[1],
		Snac_Element_Node_P( snacContext, 0, 3 )->velocity[2],
		Snac_Element_Node_P( snacContext, 0, 2 )->velocity[0],
		Snac_Element_Node_P( snacContext, 0, 2 )->velocity[1],
		Snac_Element_Node_P( snacContext, 0, 2 )->velocity[2],
		Snac_Element_Node_P( snacContext, 0, 4 )->velocity[0],
		Snac_Element_Node_P( snacContext, 0, 4 )->velocity[1],
		Snac_Element_Node_P( snacContext, 0, 4 )->velocity[2],
		Snac_Element_Node_P( snacContext, 0, 5 )->velocity[0],
		Snac_Element_Node_P( snacContext, 0, 5 )->velocity[1],
		Snac_Element_Node_P( snacContext, 0, 5 )->velocity[2],
		Snac_Element_Node_P( snacContext, 0, 7 )->velocity[0],
		Snac_Element_Node_P( snacContext, 0, 7 )->velocity[1],
		Snac_Element_Node_P( snacContext, 0, 7 )->velocity[2],
		Snac_Element_Node_P( snacContext, 0, 6 )->velocity[0],
		Snac_Element_Node_P( snacContext, 0, 6 )->velocity[1],
		Snac_Element_Node_P( snacContext, 0, 6 )->velocity[2] );


	/* Update all the elements, and in the process work out this processor's minLengthScale */
	elementIndex = 0;
	Snac_UpdateElementMomentum( (Context*)snacContext, elementIndex, &minLengthScale );
	for( elementIndex = 1; elementIndex < snacContext->mesh->elementLocalCount; elementIndex++ ) {
		double elementMinLengthScale;

		Snac_UpdateElementMomentum( (Context*)snacContext, elementIndex, &elementMinLengthScale );
		
		if( elementMinLengthScale < minLengthScale ) {
			minLengthScale = elementMinLengthScale;
		}
	}

	/* Find the global minLengthScale, and then calculate the new dt, assuming frac=0.5, strain_inert=1.0e-5f & vbc_max=3.0e-10f */
	{
		double localMinLengthScale = minLengthScale;
		
		MPI_Allreduce( &localMinLengthScale, &minLengthScale, 1, MPI_DOUBLE, MPI_MIN, CommWorld );
		snacContext->dt = minLengthScale * 0.5f * 1.0e-5f / 3.0e-10f;
	}
	printf( "Global minLengthScale: %g\n", minLengthScale );
	printf( "Dt: %g\n", snacContext->dt );
	
	/* Work out for element 0 */
	elementIndex = 0;
	Snac_StrainRate( (Context*)snacContext, elementIndex );
	SnacTest_Stress( (Context*)snacContext, elementIndex );
	for( tetraIndex = 0; tetraIndex < Tetrahedra_Count; tetraIndex++ ) {
		printf( "Element: 0, Tetrahedra: %u, Stress:  (%g %g %g) (%g %g %g)\n", tetraIndex, 
			Snac_Element_At( snacContext, 0 )->tetraStress[tetraIndex][0][0], 
			Snac_Element_At( snacContext, 0 )->tetraStress[tetraIndex][1][1], 
			Snac_Element_At( snacContext, 0 )->tetraStress[tetraIndex][2][2], 
			Snac_Element_At( snacContext, 0 )->tetraStress[tetraIndex][0][1], 
			Snac_Element_At( snacContext, 0 )->tetraStress[tetraIndex][0][2], 
			Snac_Element_At( snacContext, 0 )->tetraStress[tetraIndex][1][2] );
	}
	


	/* For each element, compare to the first element's */
	for( elementIndex = 1; elementIndex < snacContext->mesh->elementLocalCount; elementIndex++ ) {
		Bool 				error;
		
		/* For each tetrahedra of the element */
		Snac_StrainRate( (Context*)snacContext, elementIndex );
		SnacTest_Stress( (Context*)snacContext, elementIndex );
		for( tetraIndex = 0, error = False; tetraIndex < Tetrahedra_Count; tetraIndex++ ) {
			if( Snac_Element_At( snacContext, elementIndex )->tetraStress[tetraIndex][0][0] != Snac_Element_At( snacContext, 0 )->tetraStress[tetraIndex][0][0] ||
				Snac_Element_At( snacContext, elementIndex )->tetraStress[tetraIndex][1][1] != Snac_Element_At( snacContext, 0 )->tetraStress[tetraIndex][1][1] ||
				Snac_Element_At( snacContext, elementIndex )->tetraStress[tetraIndex][2][2] != Snac_Element_At( snacContext, 0 )->tetraStress[tetraIndex][2][2] ||
				Snac_Element_At( snacContext, elementIndex )->tetraStress[tetraIndex][0][1] != Snac_Element_At( snacContext, 0 )->tetraStress[tetraIndex][0][1] ||
				Snac_Element_At( snacContext, elementIndex )->tetraStress[tetraIndex][0][2] != Snac_Element_At( snacContext, 0 )->tetraStress[tetraIndex][0][2] ||
				Snac_Element_At( snacContext, elementIndex )->tetraStress[tetraIndex][1][2] != Snac_Element_At( snacContext, 0 )->tetraStress[tetraIndex][1][2] ) 
			{
				printf( "Element: %u, Tetrahedra: %u, Stress: (%g %g %g) (%g %g %g)\n", elementIndex, 
					tetraIndex,
					Snac_Element_At( snacContext, elementIndex )->tetraStress[tetraIndex][0][0], 
					Snac_Element_At( snacContext, elementIndex )->tetraStress[tetraIndex][1][1], 
					Snac_Element_At( snacContext, elementIndex )->tetraStress[tetraIndex][2][2], 
					Snac_Element_At( snacContext, elementIndex )->tetraStress[tetraIndex][0][1], 
					Snac_Element_At( snacContext, elementIndex )->tetraStress[tetraIndex][0][2], 
					Snac_Element_At( snacContext, elementIndex )->tetraStress[tetraIndex][1][2] );
				error = True;
			}
		}
		
		if( error ) {
			printf( "Element: %u, Coords: (%g %g %g), (%g %g %g), (%g %g %g), (%g %g %g), (%g %g %g) (%g %g %g) (%g %g %g) (%g %g %g)\n",
				elementIndex,
				Snac_Element_NodeCoord( snacContext, elementIndex, 0 )[0],
				Snac_Element_NodeCoord( snacContext, elementIndex, 0 )[1],
				Snac_Element_NodeCoord( snacContext, elementIndex, 0 )[2],
				Snac_Element_NodeCoord( snacContext, elementIndex, 1 )[0],
				Snac_Element_NodeCoord( snacContext, elementIndex, 1 )[1],
				Snac_Element_NodeCoord( snacContext, elementIndex, 1 )[2],
				Snac_Element_NodeCoord( snacContext, elementIndex, 3 )[0],
				Snac_Element_NodeCoord( snacContext, elementIndex, 3 )[1],
				Snac_Element_NodeCoord( snacContext, elementIndex, 3 )[2],
				Snac_Element_NodeCoord( snacContext, elementIndex, 2 )[0],
				Snac_Element_NodeCoord( snacContext, elementIndex, 2 )[1],
				Snac_Element_NodeCoord( snacContext, elementIndex, 2 )[2],
				Snac_Element_NodeCoord( snacContext, elementIndex, 4 )[0],
				Snac_Element_NodeCoord( snacContext, elementIndex, 4 )[1],
				Snac_Element_NodeCoord( snacContext, elementIndex, 4 )[2],
				Snac_Element_NodeCoord( snacContext, elementIndex, 5 )[0],
				Snac_Element_NodeCoord( snacContext, elementIndex, 5 )[1],
				Snac_Element_NodeCoord( snacContext, elementIndex, 5 )[2],
				Snac_Element_NodeCoord( snacContext, elementIndex, 7 )[0],
				Snac_Element_NodeCoord( snacContext, elementIndex, 7 )[1],
				Snac_Element_NodeCoord( snacContext, elementIndex, 7 )[2],
				Snac_Element_NodeCoord( snacContext, elementIndex, 6 )[0],
				Snac_Element_NodeCoord( snacContext, elementIndex, 6 )[1],
				Snac_Element_NodeCoord( snacContext, elementIndex, 6 )[2] );
			printf( "Element: %u, Vel: (%g %g %g), (%g %g %g), (%g %g %g), (%g %g %g), (%g %g %g) (%g %g %g) (%g %g %g) (%g %g %g)\n",
				elementIndex,
				Snac_Element_Node_P( snacContext, elementIndex, 0 )->velocity[0],
				Snac_Element_Node_P( snacContext, elementIndex, 0 )->velocity[1],
				Snac_Element_Node_P( snacContext, elementIndex, 0 )->velocity[2],
				Snac_Element_Node_P( snacContext, elementIndex, 1 )->velocity[0],
				Snac_Element_Node_P( snacContext, elementIndex, 1 )->velocity[1],
				Snac_Element_Node_P( snacContext, elementIndex, 1 )->velocity[2],
				Snac_Element_Node_P( snacContext, elementIndex, 3 )->velocity[0],
				Snac_Element_Node_P( snacContext, elementIndex, 3 )->velocity[1],
				Snac_Element_Node_P( snacContext, elementIndex, 3 )->velocity[2],
				Snac_Element_Node_P( snacContext, elementIndex, 2 )->velocity[0],
				Snac_Element_Node_P( snacContext, elementIndex, 2 )->velocity[1],
				Snac_Element_Node_P( snacContext, elementIndex, 2 )->velocity[2],
				Snac_Element_Node_P( snacContext, elementIndex, 4 )->velocity[0],
				Snac_Element_Node_P( snacContext, elementIndex, 4 )->velocity[1],
				Snac_Element_Node_P( snacContext, elementIndex, 4 )->velocity[2],
				Snac_Element_Node_P( snacContext, elementIndex, 5 )->velocity[0],
				Snac_Element_Node_P( snacContext, elementIndex, 5 )->velocity[1],
				Snac_Element_Node_P( snacContext, elementIndex, 5 )->velocity[2],
				Snac_Element_Node_P( snacContext, elementIndex, 7 )->velocity[0],
				Snac_Element_Node_P( snacContext, elementIndex, 7 )->velocity[1],
				Snac_Element_Node_P( snacContext, elementIndex, 7 )->velocity[2],
				Snac_Element_Node_P( snacContext, elementIndex, 6 )->velocity[0],
				Snac_Element_Node_P( snacContext, elementIndex, 6 )->velocity[1],
				Snac_Element_Node_P( snacContext, elementIndex, 6 )->velocity[2] );
		}
		else {
			printf( "Element %u: has same values as element 0.\n", elementIndex );
		}		
	}
	
	/* Stg_Class_Delete stuff */
	Stg_Class_Delete( snacContext );
	Stg_Class_Delete( dictionary );

	/* Close off MPI */
	MPI_Finalize();

	return 0; /* success */
}
