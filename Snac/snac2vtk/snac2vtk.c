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
**           Colin Stark, Doherty Research Scientist, Lamont-Doherty Earth Observatory (cstark@ldeo.columbia.edu)
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
*/
/** \file
** Role:
**	Converts Snac's binary output to VTK format
**
** $Id: snac2vtk.c 3270 2006-11-26 06:33:20Z EunseoChoi $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <stdio.h>
#include <stdlib.h>
/*
 * CPS mods start ...
 */
#include <math.h>
#ifndef PI
	#ifndef M_PIl
		#ifndef M_PI
			#define PI 3.14159265358979323846
		#else
			#define PI M_PI
		#endif
	#else
		#define PI M_PIl
	#endif
#endif
/*
 * ... end CPS mods
 */
#include <assert.h>

#include <limits.h>
#ifndef PATH_MAX
	#define PATH_MAX 1024
#endif


/*
 * CPS mods start ...
 */
#define ROTATE(a,i,j,k,l) g=a[i][j];h=a[k][l];a[i][j]=g-s*(h+g*tau);\
    a[k][l]=h+s*(g-h*tau);

#define NR_END 1
#define FREE_ARG char*	
#define SIGN(b) ((b) >= 0.0 ? 1 : -1)

#define SWAP(a,b,t)     t=a;a=b;b=t; 

//#define DEBUG

/*
 * ... end CPS mods
 */



void ConvertTimeStep( int rank, unsigned int dumpIteration, unsigned int simTimeStep, double time, int gnode[3], int rank_array[3], 
		      const int rankI, const int rankJ, const int rankK );

/*
 * CPS mods start ...
 */
int DerivePrincipalStresses(double stressTensor[3][3],double sp[3],double cn[3][3]);

double** dmatrix(long nrl, long nrh, long ncl, long nch);
double *dvector(long nl, long nh);
int *ivector(long nl, long nh);
void free_ivector(int *v, long nl, long nh);
void free_dmatrix(double **m, long nrl, long nrh, long ncl, long nch);
void free_dvector(double *v, long nl, long nh);
void nrerror(char error_text[]);
int jacobi(double** a, double* d, double** v);
int eigsrt(double* d, double** v);
struct stressMeasures {
    double     	principalStresses[3];
    double	eigenvectors[3][3];
    double	pressure;
    double	maxShearStress;
    double	J2;
    double	vonMisesStress;
    double	failurePotential;
    double	failureAngle;
    double	slopeShearStress;
    double	slopeNormalStress;

};
void DeriveStressMeasures(FILE *stressTensorIn, 
			  double elementStressTensor[3][3], 
			  struct stressMeasures *elementStressMeasures);

/*
 * ... end CPS mods
 */




char		path[PATH_MAX];
FILE*		strainRateIn;
FILE*		stressIn;
/*
 * CPS mods start ...
 */
FILE*		stressTensorIn;
/*
 * ... end CPS mods
 */
FILE*		pisosIn;
FILE*		coordIn;
FILE*		velIn;
FILE*		forceIn;
FILE*		phaseIn;
FILE*		tempIn;
FILE*		apsIn;
FILE*		viscIn;

unsigned int	elementLocalSize[3];
int 			doTemp = 1;
int 			doForce = 1;
int 			doAps = 1;
int 			doHPr = 1;
int 			doVisc = 1;
/*
 * CPS mods start ...
 */
double			failureAngle = -1.0;
/*
 * ... end CPS mods
 */

int main( int argc, char* argv[]) 
{
    char		tmpBuf[PATH_MAX];
    FILE*		simIn;
    FILE*		timeStepIn;
    unsigned int	rank;
    unsigned int	simTimeStep;
    unsigned int	dumpIteration;
    double		time;
    double		dt;
    int		gnode[3];
    int		rank_array[3];
    unsigned int	rankI,rankJ,rankK;
    unsigned int	stepMin=-1,stepMax=0;
	
    /*
     * CPS mods start ...
     */
    if( argc<7 || argc>10 ) {
	fprintf(stderr,"snac2vtk global-mesh-size-x global-mesh-size-y global-mesh-size-z num-processors-x num-processors-y num-processors-z [start-step[max-step]] [end-step[max-step]] [failure-angle[30 (degrees)]]\n");
	exit(1);
    }
    /*
     * ... end CPS mods
     */

    /* TODO, get from arg list */
    sprintf( path, "." );
    gnode[0] = atoi(argv[1]);
    gnode[1] = atoi(argv[2]);
    gnode[2] = atoi(argv[3]);
    rank_array[0] = atoi(argv[4]);
    rank_array[1] = atoi(argv[5]);
    rank_array[2] = atoi(argv[6]);
		
    for( rankK=0; rankK < rank_array[2]; rankK++ )
	for( rankJ=0; rankJ < rank_array[1]; rankJ++ )
	    for( rankI=0; rankI < rank_array[0]; rankI++ ) {
		rank = rankI + rankJ*rank_array[0] + rankK*rank_array[0]*rank_array[1]; 

		/* open the input files */
		sprintf( tmpBuf, "%s/sim.%u", path, rank );
		if( (simIn = fopen( tmpBuf, "r" )) == NULL ) {
		    if( rank == 0 ) {
			assert( simIn /* failed to open file for reading */ );
		    }
		    else {
			break;
		    }
		}
		sprintf( tmpBuf, "%s/timeStep.%u", path, rank );
		if( (timeStepIn = fopen( tmpBuf, "r" )) == NULL ) {
		    assert( timeStepIn /* failed to open file for reading */ );
		}
		sprintf( tmpBuf, "%s/strainRate.%u", path, rank );
		if( (strainRateIn = fopen( tmpBuf, "r" )) == NULL ) {
		    assert( strainRateIn /* failed to open file for reading */ );
		}
		sprintf( tmpBuf, "%s/stress.%u", path, rank );
		if( (stressIn = fopen( tmpBuf, "r" )) == NULL ) {
		    assert( stressIn /* failed to open file for reading */ );
		}
		/*
		 * CPS mods start ...
		 */
		sprintf( tmpBuf, "%s/stressTensor.%u", path, rank );
		if( (stressTensorIn = fopen( tmpBuf, "r" )) == NULL ) {
		    assert( stressTensorIn /* failed to open file for reading */ );
		}
		/*
		 * ... end CPS mods
		 */
		sprintf( tmpBuf, "%s/hydroPressure.%u", path, rank );
		if( ( pisosIn = fopen( tmpBuf, "r" )) == NULL ) {
		    printf( "Warning, no hydropressure.%u found... assuming the new context is not written.\n", rank );
		    doHPr = 0;
		}
		sprintf( tmpBuf, "%s/coord.%u", path, rank );
		if( (coordIn = fopen( tmpBuf, "r" )) == NULL ) {
		    assert( coordIn /* failed to open file for reading */ );
		}
		sprintf( tmpBuf, "%s/vel.%u", path, rank );
		if( (velIn = fopen( tmpBuf, "r" )) == NULL ) {
		    assert( velIn /* failed to open file for reading */ );
		}
		sprintf( tmpBuf, "%s/force.%u", path, rank );
		if( (forceIn = fopen( tmpBuf, "r" )) == NULL ) {
		    fprintf(stderr, "Warning, no force.%u found... assuming force outputting not enabled.\n", rank );
		    doForce = 0;
		}
		sprintf( tmpBuf, "%s/phaseIndex.%u", path, rank );
		if( (phaseIn = fopen( tmpBuf, "r" )) == NULL ) {
		    assert( phaseIn /* failed to open file for reading */ );
		}
		sprintf( tmpBuf, "%s/temperature.%u", path, rank );
		if( (tempIn = fopen( tmpBuf, "r" )) == NULL ) {
		    fprintf(stderr, "Warning, no temperature.%u found... assuming temperature plugin not used.\n", rank );
		    doTemp = 0;
		}
		sprintf( tmpBuf, "%s/plStrain.%u", path, rank );
		if( (apsIn = fopen( tmpBuf, "r" )) == NULL ) {
		    fprintf(stderr, "Warning, no plStrain.%u found... assuming plastic plugin not used.\n", rank );
		    doAps = 0;
		}
		sprintf( tmpBuf, "%s/viscosity.%u", path, rank );
		if( (viscIn = fopen( tmpBuf, "r" )) == NULL ) {
		    fprintf(stderr, "Warning, no viscosity.%u found... assuming maxwell plugin not used.\n", rank );
		    doVisc = 0;
		}
		
		
		/* Read in simulation information... TODO: assumes nproc=1 */
		fscanf( simIn, "%u %u %u\n", &elementLocalSize[0], &elementLocalSize[1], &elementLocalSize[2] );

		
		/*
		 * CPS mods start ...
		 */
		/* 		if( feof(timeStepIn) ) { */
		/* 			fprintf(stderr, "Time step file zero length\n" ); */
		/* 			assert(timeStepIn); */
		/* 		} else { */
		/* 		    fscanf( timeStepIn, "%16u %16lg %16lg\n", &simTimeStep, &time, &dt ); */
		/* 		} */
		while( !feof( timeStepIn ) ) {
		    fscanf( timeStepIn, "%16u %16lg %16lg\n", &simTimeStep, &time, &dt );
		    /* 			fprintf(stderr, "Time step:  %u <-> %g\n", simTimeStep, time ); */
		    if(stepMin==-1) stepMin=simTimeStep;
		    if(stepMax<simTimeStep) stepMax=simTimeStep;
		}
		if( stepMin==-1 ) {
		    fprintf(stderr, "Time step file zero length\n" );
		    assert(stepMin);
		}
		/* 		fseek( timeStepIn, 0, SEEK_SET ); */
		rewind(timeStepIn);
		
		if(argc>=8) {
		    if(atoi(argv[7])>stepMin) stepMin=atoi(argv[7]);
		} else {
		    stepMin=stepMax;
		}
		if(argc>=9) {
		    if(atoi(argv[8])<stepMax) stepMax=atoi(argv[8]);
		}/*  else { */
		/* 		    stepMax=stepMin; */
		/* 		} */

		fprintf(stderr, "Time step range:  %u <-> %u\n", stepMin, stepMax );

		/*
		 *  Parse angle used to compute failure potential - using global variable (ick)
		 */
		if(argc>=10) {
		    failureAngle=atof(argv[9]);
		    fprintf(stderr, "Failure angle = %g\n", failureAngle );
		}

		/* Read in loop information */
		dumpIteration = 0;
		while( !feof( timeStepIn ) ) {
		    fscanf( timeStepIn, "%16u %16lg %16lg\n", &simTimeStep, &time, &dt );
		    if( simTimeStep <stepMin || simTimeStep > stepMax ) {
			dumpIteration++;
			continue;
		    }
		    ConvertTimeStep( rank, dumpIteration, simTimeStep, time, gnode, rank_array, rankI, rankJ, rankK );
		    dumpIteration++;
		}
		/*
		 * ... end CPS mods
		 */
		
		/* Close the input files */
		if( apsIn ) {
		    fclose( apsIn );
		}
		if( viscIn ) {
		    fclose( viscIn );
		}
		if( tempIn ) {
		    fclose( tempIn );
		}
		if( forceIn ) {
		    fclose( forceIn );
		}
		if( pisosIn ) {
		    fclose( pisosIn );
		}
		fclose( phaseIn );
		fclose( velIn );
		fclose( coordIn );
		fclose( stressIn );
		/*
		 * CPS mods start ...
		 */
		fclose( stressTensorIn );
		/*
		 * ... end CPS mods
		 */
		fclose( strainRateIn );
		fclose( timeStepIn );
		fclose( simIn );
		
		/* do next rank */
		rank++;
	    }

    return 0;
}


void ConvertTimeStep( int rank, unsigned int dumpIteration, unsigned int simTimeStep, double time, int gnode[3], int rank_array[3], const int rankI, const int rankJ, const int rankK ) 
{
    char		tmpBuf[PATH_MAX], tmpBuf1[PATH_MAX];
    FILE*		vtkOut;
    FILE*		vtkOut1;
    unsigned int	elementLocalCount = elementLocalSize[0] * elementLocalSize[1] * elementLocalSize[2];
    unsigned int	nodeLocalSize[3] = { elementLocalSize[0] + 1, elementLocalSize[1] + 1, elementLocalSize[2] + 1 };
    unsigned int	nodeLocalCount = nodeLocalSize[0] * nodeLocalSize[1] * nodeLocalSize[2];
    unsigned int	node_gI;
    unsigned int	element_gI;
	
    /* open the output file */
    sprintf( tmpBuf, "%s/snac.%i.%06u.vts", path, rank, simTimeStep );
    if( (vtkOut = fopen( tmpBuf, "w+" )) == NULL ) {
	assert( vtkOut /* failed to open file for writing */ );
    }
	
    /* Write out simulation information */
    fprintf( vtkOut, "<?xml version=\"1.0\"?>\n" );
    fprintf( vtkOut, "<VTKFile type=\"StructuredGrid\"  version=\"0.1\" byte_order=\"LittleEndian\" compressor=\"vtkZLibDataCompressor\">\n");
    fprintf( vtkOut, "  <StructuredGrid WholeExtent=\"%d %d %d %d %d %d\">\n",
	     rankI*elementLocalSize[0],(rankI+1)*elementLocalSize[0],
	     rankJ*elementLocalSize[1],(rankJ+1)*elementLocalSize[1],
	     rankK*elementLocalSize[2],(rankK+1)*elementLocalSize[2]);
    fprintf( vtkOut, "    <Piece Extent=\"%d %d %d %d %d %d\">\n",
	     rankI*elementLocalSize[0],(rankI+1)*elementLocalSize[0],
	     rankJ*elementLocalSize[1],(rankJ+1)*elementLocalSize[1],
	     rankK*elementLocalSize[2],(rankK+1)*elementLocalSize[2]);
	
    /* Start the node section */
    fprintf( vtkOut, "      <PointData Vectors=\"velocity\">\n");
	
    /* Write out the velocity information */
    fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"velocity\" NumberOfComponents=\"3\" format=\"ascii\">\n");
    fseek( velIn, dumpIteration * nodeLocalCount * sizeof(float) * 3, SEEK_SET );
    for( node_gI = 0; node_gI < nodeLocalCount; node_gI++ ) {
	float		vel[3];
	fread( &vel, sizeof(float), 3, velIn );
	fprintf( vtkOut, "%g %g %g\n", vel[0], vel[1], vel[2] );
    }
    fprintf( vtkOut, "        </DataArray>\n");
	
    /* Write out the force information */
    if( doForce ) {
	fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"force\" NumberOfComponents=\"3\" format=\"ascii\">\n");
	fseek( forceIn, dumpIteration * nodeLocalCount * sizeof(float) * 3, SEEK_SET );
	for( node_gI = 0; node_gI < nodeLocalCount; node_gI++ ) {
	    float		force[3];
	    fread( &force, sizeof(float), 3, forceIn );
	    fprintf( vtkOut, "%g %g %g\n", force[0], force[1], force[2] );
	}
	fprintf( vtkOut, "        </DataArray>\n");
    }
	
    /* Write out the temperature information */
    if( doTemp ) {
	fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"temperature\" format=\"ascii\">\n");
	fseek( tempIn, dumpIteration * nodeLocalCount * sizeof(float), SEEK_SET );
	for( node_gI = 0; node_gI < nodeLocalCount; node_gI++ ) {
	    float		temperature;
	    fread( &temperature, sizeof(float), 1, tempIn );
	    fprintf( vtkOut, "%g ", temperature );
	}
	fprintf( vtkOut, "        </DataArray>\n");
    }
    fprintf( vtkOut, "      </PointData>\n");

    /* Start the element section */
    fprintf( vtkOut, "      <CellData Scalars=\"strainRate\">\n");
	
    /* Write out the strain rate information */
    fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"strainRate\" format=\"ascii\">\n");
    fseek( strainRateIn, dumpIteration * elementLocalCount * sizeof(float), SEEK_SET );
    for( element_gI = 0; element_gI < elementLocalCount; element_gI++ ) {
	float		strainRate;
	fread( &strainRate, sizeof(float), 1, strainRateIn );
	fprintf( vtkOut, "%g ", strainRate );
    }
    fprintf( vtkOut, "        </DataArray>\n");
	
    /* Write out the stress information */
    fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"stress\" format=\"ascii\">\n");
    fseek( stressIn, dumpIteration * elementLocalCount * sizeof(float), SEEK_SET );
    for( element_gI = 0; element_gI < elementLocalCount; element_gI++ ) {
	float		stress;
	fread( &stress, sizeof(float), 1, stressIn );
	fprintf( vtkOut, "%g ", stress );
    }
    fprintf( vtkOut, "        </DataArray>\n");

    /*
     * CPS mods start ...
     */
    /* Write out the maxShearStress information */
    fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"maxShearStress\" format=\"ascii\">\n");
    fseek( stressTensorIn, dumpIteration * elementLocalCount * sizeof(float)*9*10, SEEK_SET );
    for( element_gI = 0; element_gI < elementLocalCount; element_gI++ ) {
	double	        	elementStressTensor[3][3]={{0,0,0},{0,0,0},{0,0,0}};
	struct stressMeasures	elementStressMeasures;
	/*
	 *  Build average stress tensor for element and derive useful stress measures
	 */
	DeriveStressMeasures(stressTensorIn, 
			     elementStressTensor,  &elementStressMeasures);
	/*
	 *  Write the chosen derived stress value to Paraview file
	 */
	fprintf( vtkOut, "%g ", elementStressMeasures.maxShearStress ); 
#ifdef DEBUG
	if (element_gI<10) fprintf( stderr, "Element max shear stress %d: %g\n", element_gI, elementStressMeasures.maxShearStress ); 
#endif	
    }
    fprintf( vtkOut, "        </DataArray>\n");

    /* Write out the vonMisesStress information */
    fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"vonMisesStress\" format=\"ascii\">\n");
    fseek( stressTensorIn, dumpIteration * elementLocalCount * sizeof(float)*9*10, SEEK_SET );
    for( element_gI = 0; element_gI < elementLocalCount; element_gI++ ) {
	double	        	elementStressTensor[3][3]={{0,0,0},{0,0,0},{0,0,0}};
	struct stressMeasures	elementStressMeasures;
	/*
	 *  Build average stress tensor for element and derive useful stress measures
	 */
	DeriveStressMeasures(stressTensorIn, elementStressTensor, &elementStressMeasures);
	/*
	 *  Write the chosen derived stress value to Paraview file
	 */
	fprintf( vtkOut, "%g ", elementStressMeasures.vonMisesStress ); 
#ifdef DEBUG
	if (element_gI<10) fprintf( stderr, "Element von Mises stress %d: %g\n", element_gI, elementStressMeasures.vonMisesStress ); 
#endif	
    }
    fprintf( vtkOut, "        </DataArray>\n");

    /* Write out the slopeShearStress information */
    fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"slopeShearStress\" format=\"ascii\">\n");
    fseek( stressTensorIn, dumpIteration * elementLocalCount * sizeof(float)*9*10, SEEK_SET );
    for( element_gI = 0; element_gI < elementLocalCount; element_gI++ ) {
	double	        	elementStressTensor[3][3]={{0,0,0},{0,0,0},{0,0,0}};
	struct stressMeasures	elementStressMeasures;
	/*
	 *  Build average stress tensor for element and derive useful stress measures
	 */
	elementStressMeasures.failureAngle=(failureAngle<0.0 ? 0.0 : failureAngle);
	DeriveStressMeasures(stressTensorIn, elementStressTensor, &elementStressMeasures);
	/*
	 *  Write the chosen derived stress value to Paraview file
	 */
	fprintf( vtkOut, "%g ", elementStressMeasures.slopeShearStress );
#ifdef DEBUG
	if (element_gI<10) fprintf( stderr, "Element slope shear stress %d: %g  at angle %g\n", element_gI, elementStressMeasures.slopeShearStress, elementStressMeasures.failureAngle); 
#endif	
    }
    fprintf( vtkOut, "        </DataArray>\n");

    /* Write out the slopeNormalStress information */
    fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"slopeNormalStress\" format=\"ascii\">\n");
    fseek( stressTensorIn, dumpIteration * elementLocalCount * sizeof(float)*9*10, SEEK_SET );
    for( element_gI = 0; element_gI < elementLocalCount; element_gI++ ) {
	double	        	elementStressTensor[3][3]={{0,0,0},{0,0,0},{0,0,0}};
	struct stressMeasures	elementStressMeasures;
	/*
	 *  Build average stress tensor for element and derive useful stress measures
	 */
	elementStressMeasures.failureAngle=(failureAngle<0.0 ? 0.0 : failureAngle);
	DeriveStressMeasures(stressTensorIn, elementStressTensor, &elementStressMeasures);
	/*
	 *  Write the chosen derived stress value to Paraview file
	 */
	fprintf( vtkOut, "%g ", elementStressMeasures.slopeNormalStress );
#ifdef DEBUG
	if (element_gI<10) fprintf( stderr, "Element slope normal stress %d: %g  at angle %g\n", element_gI, elementStressMeasures.slopeNormalStress, elementStressMeasures.failureAngle); 
#endif	
    }
    fprintf( vtkOut, "        </DataArray>\n");

    /* Write out the failurePotential information */
    fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"failurePotential\" format=\"ascii\">\n");
    fseek( stressTensorIn, dumpIteration * elementLocalCount * sizeof(float)*9*10, SEEK_SET );
    for( element_gI = 0; element_gI < elementLocalCount; element_gI++ ) {
	double	        	elementStressTensor[3][3]={{0,0,0},{0,0,0},{0,0,0}};
	struct stressMeasures	elementStressMeasures;
	/*
	 *  Build average stress tensor for element and derive useful stress measures
	 */
	elementStressMeasures.failureAngle=(failureAngle<0.0 ? 0.0 : failureAngle);
	DeriveStressMeasures(stressTensorIn, elementStressTensor, &elementStressMeasures);
	/*
	 *  Write the chosen derived stress value to Paraview file
	 */
	fprintf( vtkOut, "%g ", elementStressMeasures.failurePotential );
#ifdef DEBUG
	if (element_gI<10) fprintf( stderr, "Element failure potential %d: %g  at angle %g\n", element_gI, elementStressMeasures.failurePotential, elementStressMeasures.failureAngle); 
#endif	
    }
    fprintf( vtkOut, "        </DataArray>\n");
    /*
     * ... end CPS mods
     */
	




    /* Write out the phase information */
    fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"phase\" format=\"ascii\">\n");
    fseek( phaseIn, dumpIteration * elementLocalCount * sizeof(unsigned int), SEEK_SET );
    for( element_gI = 0; element_gI < elementLocalCount; element_gI++ ) {
	unsigned int		phaseIndex;
	fread( &phaseIndex, sizeof(unsigned int), 1, phaseIn );
	fprintf( vtkOut, "%u ", phaseIndex );
    }
    fprintf( vtkOut, "        </DataArray>\n");

    /* Write out the plastic strain information */
    if( doAps ) {
	fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"plStrain\" format=\"ascii\">\n");
	fseek( apsIn, dumpIteration * elementLocalCount * sizeof(float), SEEK_SET );
	for( element_gI = 0; element_gI < elementLocalCount; element_gI++ ) {
	    float		plStrain;
	    fread( &plStrain, sizeof(float), 1, apsIn );
	    fprintf( vtkOut, "%g ", plStrain );
	}
	fprintf( vtkOut, "        </DataArray>\n");
    }

    /* Write out the pressure information */
    if( doHPr ) {
	fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"pressure\" format=\"ascii\">\n");
	fseek( pisosIn, dumpIteration * elementLocalCount * sizeof(float), SEEK_SET );
	for( element_gI = 0; element_gI < elementLocalCount; element_gI++ ) {
	    float		pressure;
	    fread( &pressure, sizeof(float), 1, pisosIn );
	    fprintf( vtkOut, "%g ", pressure );
	}
	fprintf( vtkOut, "        </DataArray>\n");
    }

    /* Write out the pressure information */
    if( doVisc ) {
	fprintf( vtkOut, "        <DataArray type=\"Float32\" Name=\"viscosity\" format=\"ascii\">\n");
	fseek( viscIn, dumpIteration * elementLocalCount * sizeof(float), SEEK_SET );
	for( element_gI = 0; element_gI < elementLocalCount; element_gI++ ) {
	    float		viscosity;
	    fread( &viscosity, sizeof(float), 1, viscIn );
	    fprintf( vtkOut, "%g ", viscosity );
	}
	fprintf( vtkOut, "        </DataArray>\n");
    }
    fprintf( vtkOut, "      </CellData>\n");
	
    /* Write out coordinates. */
    fprintf( vtkOut, "      <Points>\n");
    fprintf( vtkOut, "        <DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">\n");
    fseek( coordIn, dumpIteration * nodeLocalCount * sizeof(float) * 3, SEEK_SET );
    for( node_gI = 0; node_gI < nodeLocalCount; node_gI++ ) {
	float		coord[3];
	fread( &coord, sizeof(float), 3, coordIn );
	fprintf( vtkOut, "%g %g %g\n", coord[0], coord[1], coord[2] );
    }
    fprintf( vtkOut, "        </DataArray>\n");
    fprintf( vtkOut, "      </Points>\n");
    fprintf( vtkOut, "    </Piece>\n");
    fprintf( vtkOut, "  </StructuredGrid>\n");
    fprintf( vtkOut, "</VTKFile>\n");

    /* Close the output file */
    fclose( vtkOut );

    /* Write out Parallel VTS file. Only once when rank == 0. */
    if( rank == 0 ) {
	int rankII, rankJJ, rankKK, rank2;

	sprintf( tmpBuf1, "%s/snac.%06u.pvts", path, simTimeStep );
	if( (vtkOut1 = fopen( tmpBuf1, "w" )) == NULL ) {
	    assert( vtkOut1 /* failed to open file for writing */ );
	}
	fprintf(stderr,"Writing file %s...\n",tmpBuf1);

	fprintf( vtkOut1, "<?xml version=\"1.0\"?>\n" );
	fprintf( vtkOut1, "<VTKFile type= \"PStructuredGrid\"  version= \"0.1\" byte_order=\"LittleEndian\" compressor=\"vtkZLibDataCompressor\">\n");
	fprintf( vtkOut1, "  <PStructuredGrid WholeExtent=\"0 %d 0 %d 0 %d\" GhostLevel=\"0\">\n",gnode[0]-1,gnode[1]-1,gnode[2]-1);

	/* Start the node section */
	fprintf( vtkOut1, "    <PPointData>\n");
	fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"velocity\" NumberOfComponents=\"3\"/>\n");
	if( doForce )
	    fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"force\" NumberOfComponents=\"3\"/>\n");
	if( doTemp )
	    fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"temperature\"/>\n");
	fprintf( vtkOut1, "    </PPointData>\n");

	/* Start the element section */
	fprintf( vtkOut1, "    <PCellData>\n");
	fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"strainRate\"/>\n");
	fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"stress\"/>\n");
	/*
	 * CPS mods start ...
	 */
	fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"maxShearStress\"/>\n");
	fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"vonMisesStress\"/>\n");
	fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"slopeShearStress\"/>\n");
	fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"slopeNormalStress\"/>\n");
	fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"failurePotential\"/>\n");
	/*
	 * ... end CPS mods
	 */
	fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"phase\"/>\n");
	if( doAps )
	    fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"plStrain\"/>\n");
	if( doHPr )
	    fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"pressure\"/>\n");
	if( doVisc )
	    fprintf( vtkOut1, "        <PDataArray type=\"Float32\" Name=\"viscosity\"/>\n");
	fprintf( vtkOut1, "    </PCellData>\n");
	
	/* Write out coordinates. */
	fprintf( vtkOut1, "    <PPoints>\n");
	fprintf( vtkOut1, "      <PDataArray type=\"Float32\" NumberOfComponents=\"3\"/>\n");
	fprintf( vtkOut1, "    </PPoints>\n");

	/* Write pieces that actually contains data.*/
	for( rankII=0; rankII < rank_array[0]; rankII++ )
	    for( rankJJ=0; rankJJ < rank_array[1]; rankJJ++ )
		for( rankKK=0; rankKK < rank_array[2]; rankKK++ ) {
		    rank2 = rankII + rankJJ*rank_array[0] + rankKK*rank_array[0]*rank_array[1]; 
		    fprintf( vtkOut1, "    <Piece Extent=\"%d %d %d %d %d %d\" Source=\"%s/snac.%d.%06u.vts\"/>\n",
			     rankII*elementLocalSize[0],(rankII+1)*elementLocalSize[0],
			     rankJJ*elementLocalSize[1],(rankJJ+1)*elementLocalSize[1],
			     rankKK*elementLocalSize[2],(rankKK+1)*elementLocalSize[2],
			     path, rank2, simTimeStep );
		}
	
	fprintf( vtkOut1, "  </PStructuredGrid>\n");
	fprintf( vtkOut1, "</VTKFile>\n");
	/* Close the output file. */
	fclose( vtkOut1 );
    }
}









/*
 * CPS mods start ...
 */

int DerivePrincipalStresses(double stressTensor[3][3], double sp[3], double cn[3][3])
{

    double **a,**v,*d;
    int i,j;

    a = dmatrix(1,3,1,3);
    v = dmatrix(1,3,1,3);
    d = dvector(1,3);

    a[1][1] = stressTensor[0][0];
    a[2][2] = stressTensor[1][1];
    a[3][3] = stressTensor[2][2];
    a[1][2] = stressTensor[0][1];
    a[1][3] = stressTensor[0][2];
    a[2][3] = stressTensor[1][2];
    a[2][1] = a[1][2];
    a[3][1] = a[1][3];
    a[3][2] = a[2][3];

    jacobi(a,d,v);

    d[1] *= -1.0f;
    d[2] *= -1.0f;
    d[3] *= -1.0f;

    eigsrt(d,v);

    d[1] *= -1.0f;
    d[2] *= -1.0f;
    d[3] *= -1.0f;

    for(i=0;i<3;i++) {
	sp[i] = d[i+1];
	for(j=0;j<3;j++) {
	    cn[i][j] = v[j+1][i+1];
	}
    }

    free_dmatrix(a,1,3,1,3);
    free_dmatrix(v,1,3,1,3);
    free_dvector(d,1,3);

    return(1);
}

int jacobi(double** a, double* d, double** v)
{

    int nrot = 0;
    const unsigned int nmax = 100, n = 3;
    double b[nmax], z[nmax], tresh,sm,g,h,t,theta,c,s,tau;

    int i,j,ip,iq;

    for(ip=1;ip<=n;ip++) {
	for(iq=1;iq<=n;iq++) v[ip][iq] = 0.0f;
	v[ip][ip] = 1.0f;
    }

    for(ip=1;ip<=n;ip++) {
	b[ip] = d[ip] = a[ip][ip];
	z[ip] = 0.0f;
    }

    for(i=1;i<=50;i++) {
	sm = 0.0f;
	for(ip=1;ip<=n-1;ip++) {
	    for(iq=ip+1;iq<=n;iq++)
		sm += fabs(a[ip][iq]);
	}
	if(sm == 0.0f)
	    return(0);

	if(i < 4) {
	    tresh = 0.2f * sm / ( n*n );
	}
	else {
	    tresh = 0.0f;
	}

	for(ip=1;ip<=n-1;ip++) {
	    for(iq=ip+1;iq<=n;iq++) {
		g = 100.0f*fabs(a[ip][iq]);
		if( (i > 4) && (double)(fabs(d[ip])+g) == (double)fabs(d[ip]) && (double)(fabs(d[iq])+g) == (double)fabs(d[iq]))
		    a[ip][iq] = 0.0f;
		else if( fabs(a[ip][iq]) > tresh ) {
		    h = d[iq] - d[ip];
		    if( (fabs(h)+g) == fabs(h) )
			t = a[ip][iq] / h;
		    else {
			theta = 0.5f * h / (a[ip][iq]);
			t = 1.0f / (fabs(theta) + sqrt(1.0f + theta*theta));
			if(theta < 0.0f) t *= -1.0f;
		    }
		    c = 1.0f / sqrt(1.0f + t*t);
		    s = t * c;
		    tau = s / (1.0f + c);
		    h = t * a[ip][iq];
		    z[ip] -= h;
		    z[iq] += h;
		    d[ip] -= h;
		    d[iq] += h;
		    a[ip][iq] = 0.0f;
		    for(j=1;j<=ip-1;j++) {
			ROTATE(a,j,ip,j,iq);
		    }
		    for(j=ip+1;j<=iq-1;j++) {
			ROTATE(a,ip,j,j,iq);
		    }
		    for(j=iq+1;j<=n;j++) {
			ROTATE(a,ip,j,iq,j);
		    }
		    for(j=1;j<=n;j++) {
			ROTATE(v,j,ip,j,iq);
		    }

		    ++nrot;
		}
	    }
	}
	for(ip=1;ip<=n;ip++) {
	    b[ip] += z[ip];
	    d[ip] = b[ip];
	    z[ip] = 0.0f;
	}
    }
    assert(i<50);

    return 1;
}


int eigsrt(double* d, double** v)
{

	const unsigned int n = 3;
	int i,j,k;
	double p;

	for(i=1;i<n;i++) {
		k = i;
		p = d[i];

		for(j=i+1;j<=n;j++) {
			if(d[j] >= p) {
				k = j;
				p = d[j];
			}
		}
		if(k != i) {
			d[k] = d[i];
			d[i] = p;
			for(j=1;j<=n;j++) {
				p = v[j][i];
				v[j][i] = v[j][k];
				v[j][k] = p;
			}
		}
	}
	return(0);
}

double **dmatrix(long nrl, long nrh, long ncl, long nch)
	/* allocate a double matrix with subscript range m[nrl..nrh][ncl..nch] */
{
    long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
    double **m;

    /* allocate pointers to rows */
    m=(double **) malloc((size_t)((nrow+NR_END)*sizeof(double*)));
    if (!m) nrerror("allocation failure 1 in matrix()");
    m += NR_END;
    m -= nrl;
    /* allocate rows and set pointers to them */
    m[nrl]=(double *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double)));
    if (!m[nrl]) nrerror("allocation failure 2 in matrix()");
    m[nrl] += NR_END;
    m[nrl] -= ncl;
    for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;
    /* return pointer to array of pointers to rows */
    return m;
}


void free_dmatrix(double **m, long nrl, long nrh, long ncl, long nch)
	/* free a double matrix allocated by dmatrix() */
{
    free((FREE_ARG) (m[nrl]+ncl-NR_END));
    free((FREE_ARG) (m+nrl-NR_END));
}


void nrerror(char error_text[])
{
    fprintf(stderr,"Run-time error...\n");
    fprintf(stderr,"%s\n",error_text);
    assert(1);
}


double *dvector(long nl, long nh)
	/* allocate a double vector with subscript range v[nl..nh] */
{
    double *v;
    v=(double *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(double)));
    if (!v) nrerror("allocation failure in dvector()");
    return v-nl+NR_END;
}

int *ivector(long nl, long nh)
	/* allocate an int vector with subscript range v[nl..nh] */
{
    int *v;
    v=(int *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(int)));
    if (!v) nrerror("allocation failure in ivector()");
    return v-nl+NR_END;
}

void free_dvector(double *v, long nl, long nh)
	/* free a double vector allocated with dvector() */
{
    free((FREE_ARG) (v+nl-NR_END));
}

void free_ivector(int *v, long nl, long nh)
	/* free an int vector allocated with ivector() */
{
    free((FREE_ARG) (v+nl-NR_END));
}



/*
 *----------------------------------------------------------------------
 *
 * DeriveStressMeasures --
 *
 *      Process tetrahedral stress tensors into element stress measures
 *
 * Returns:
 *      Void
 *
 * Side effects:
 *      Puts stress measures in variables passed to it
 *
 *----------------------------------------------------------------------
 */
void 
DeriveStressMeasures(FILE *stressTensorIn, double elementStressTensor[3][3], struct stressMeasures *elementStressMeasures)
{
    float		stressTensorArray[10][3][3];
    int			tetra_I;
    const int		numberTetrahedra=10;
    double		failureAngle=elementStressMeasures->failureAngle*M_PI/180.0;
    double		normalVector[3],slopeParallelVector[3],tractionVector[3];
    double		tmp;

    /*
     *  Read all tetrahedral stress tensors for this element gI
     */
    fread( stressTensorArray, sizeof(float)*9*numberTetrahedra, 1, stressTensorIn );
    for( tetra_I = 0; tetra_I < 10; tetra_I++ ) {
	/*
	 *  Build average stress tensor for element by summing tetrahedral tensor components
	 *   - even though it's symmetric, do for all 9 components in case we pick the wrong ones before diagonalization
	 */
	elementStressTensor[0][0]+=stressTensorArray[tetra_I][0][0]/(double)numberTetrahedra;
	elementStressTensor[1][1]+=stressTensorArray[tetra_I][1][1]/(double)numberTetrahedra;
	elementStressTensor[2][2]+=stressTensorArray[tetra_I][2][2]/(double)numberTetrahedra;
	elementStressTensor[0][1]+=stressTensorArray[tetra_I][0][1]/(double)numberTetrahedra;
	elementStressTensor[0][2]+=stressTensorArray[tetra_I][0][2]/(double)numberTetrahedra;
	elementStressTensor[1][2]+=stressTensorArray[tetra_I][1][2]/(double)numberTetrahedra;
	elementStressTensor[1][0]+=stressTensorArray[tetra_I][1][0]/(double)numberTetrahedra;
	elementStressTensor[2][0]+=stressTensorArray[tetra_I][2][0]/(double)numberTetrahedra;
	elementStressTensor[2][1]+=stressTensorArray[tetra_I][2][1]/(double)numberTetrahedra;
    }
    /*
     *  Diagonalize and find principal stresses from mean stress tensor for element
     */
    DerivePrincipalStresses(elementStressTensor,elementStressMeasures->principalStresses,elementStressMeasures->eigenvectors);
    SWAP (elementStressMeasures->principalStresses[0], elementStressMeasures->principalStresses[2], tmp);  /*  Put sigma1 and sigma3 in order */
    /*
     *  Calculate pressure as sum sigma1+sigma2+sigma3
     */
    elementStressMeasures->pressure = (elementStressMeasures->principalStresses[0]
				       +elementStressMeasures->principalStresses[1]
				       +elementStressMeasures->principalStresses[2])/3.0; 
    /*
     *  Calculate maximum shear stress sigma1-sigma3
     */
    if(elementStressMeasures->principalStresses[2]>elementStressMeasures->principalStresses[0]) fprintf(stderr,"[2]>[0]\n");
    elementStressMeasures->maxShearStress = (elementStressMeasures->principalStresses[0]-elementStressMeasures->principalStresses[2]);
    /*
     *  Calculate 2nd deviatoric stress invariant
     */
    elementStressMeasures->J2 = 
	(pow((elementStressMeasures->principalStresses[0]-elementStressMeasures->principalStresses[1]),2.0)
	 +pow((elementStressMeasures->principalStresses[1]-elementStressMeasures->principalStresses[2]),2.0)
	 +pow((elementStressMeasures->principalStresses[2]-elementStressMeasures->principalStresses[0]),2.0) )/6.0;
    /*
     *  Calculate the useful form of J2, the von Mises stress
     */
    elementStressMeasures->vonMisesStress = sqrt(3.0*elementStressMeasures->J2);
    /*
     *  Calculate the traction vector on hillslope and related shear and normal stresses on that dipping surface (down to left)
     */
    normalVector[0] = -sin(failureAngle);
    normalVector[1] = cos(failureAngle);
    normalVector[2] = 0.0;
    slopeParallelVector[0] = -cos(failureAngle);
    slopeParallelVector[1] = -sin(failureAngle);
    slopeParallelVector[2] = 0.0;
    tractionVector[0] = ( elementStressTensor[0][0]*normalVector[0] + elementStressTensor[1][0]*normalVector[1] + elementStressTensor[2][0]*normalVector[2] );
    tractionVector[1] = ( elementStressTensor[0][1]*normalVector[0] + elementStressTensor[1][1]*normalVector[1] + elementStressTensor[2][1]*normalVector[2] );
    tractionVector[2] = ( elementStressTensor[0][2]*normalVector[0] + elementStressTensor[1][2]*normalVector[1] + elementStressTensor[2][2]*normalVector[2] );
    elementStressMeasures->slopeShearStress 
    	= tractionVector[0]*slopeParallelVector[0] + tractionVector[1]*slopeParallelVector[1] + tractionVector[2]*slopeParallelVector[2];
    elementStressMeasures->slopeNormalStress 
    	= tractionVector[0]*normalVector[0] + tractionVector[1]*normalVector[1] + tractionVector[2]*normalVector[2];

    /*
     *  Calculate the failure potential for hillslope angle
     */
/*     elementStressMeasures->failurePotential= ( (fabs(elementStressMeasures->maxShearStress)-1e6) */
/* 					       /(-2*elementStressMeasures->pressure/3.0) ); */
    elementStressMeasures->failurePotential= fabs(-elementStressMeasures->slopeShearStress/elementStressMeasures->slopeNormalStress);
					       

}



/*
 * ... end CPS mods
 */
