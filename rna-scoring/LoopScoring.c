#include <stdio.h>
#include "LoopScoring.h"
#include "TreeScoring.h"

char bases[16] = {0, 'A', 'C', 'M', 'G', 'R', 'S', 'V', 'U',
                      'W', 'Y', 'H', 'K', 'D', 'B', 'N'};

int eL(int i, int j, int ip, int jp, int* RNA, nndb_constants* param) {
    //ZS: internal loop calculations, borrowed from GTfold, with small 
    //modifications (eg. deleted eparam's which nobody understood and 
    //they were zero to our knowledge anyway) 
	int energy;
	int size1, size2, size;
	int loginc;
	int lopsided; /* define the asymmetry of an interior loop */

	energy = INFINITY_;
	loginc = 0;

	size1 = ip - i - 1;
	size2 = j - jp - 1;
	size = size1 + size2;

	if (size1 == 0 || size2 == 0) {
		if (size > 30) {
			loginc = (int) floor(param->prelog * log((double) size / 30.0));
			//(Note: auPen is defined in LoopScoring.h and returns 0 if no AU 
			//penalty should be applied, and 1 if it should be applied. This is 
			//different from how it is in GTfold.)	
			energy = param->bulge[30] + loginc + 
						param->auend*(auPen(RNA[i], RNA[j]) + auPen(RNA[ip], RNA[jp]));	
		} else if (size <= 30 && size != 1) {
			energy = param->bulge[size];
			energy += param->auend*(auPen(RNA[i], RNA[j]) + auPen(RNA[ip], RNA[jp]));
		} else if (size == 1) {
			energy = param->stack[fourBaseIndex(RNA[i], RNA[j], RNA[ip], RNA[jp])]
			               + param->bulge[size];
		}
	} else {
		//Internal loop
		lopsided = abs(size1 - size2);
		if (size > 30) {
			loginc = (int) floor(param->prelog * log((double) size / 30.0));
			//ZS: Somebody else's previous comment follows, I don't know the answer: 
			/* Please check what should be the difference in the following two options. Is it correct?*/
			if (!((size1 == 1 || size2 == 1) && param->gail)) { 
				//ZS: gail = grossly assymetric interior loop rule, ON(1)/OFF(0)
				energy = param->tstki[fourBaseIndex(RNA[i], RNA[j], RNA[i + 1], RNA[j - 1])]
						 	+ param->tstki[fourBaseIndex(RNA[jp], RNA[ip], RNA[jp + 1], 
							 RNA[ip - 1])] + param->inter[30] + loginc + param->eparam[3]   //ZS: I have no idea about eparam here 
							  + MIN(param->maxpen, (lopsided * param->poppen[MIN(2, MIN(size1, size2))]));
			} else {
				energy = param->tstki[fourBaseIndex(RNA[i], RNA[j], BASE_A, BASE_A)]
				        + param->tstki[fourBaseIndex(RNA[jp], RNA[ip], BASE_A,
				        		BASE_A)] + param->inter[30] + loginc + param->eparam[3] + 
								  MIN(param->maxpen, (lopsided	* param->poppen[MIN(2, MIN(size1, size2))]));
			}
		}
		/* if size is not > 30, we have a lot of cases... */
		else if (size1 == 2 && size2 == 2) {
			/* 2x2 internal loop */
			energy = param->iloop22[RNA[i]][RNA[ip]][RNA[j]][RNA[jp]][RNA[i + 1]][RNA[i+ 2]][RNA[j - 1]][RNA[j - 2]];
		} else if (size1 == 1 && size2 == 2) {
			energy = param->iloop21[RNA[i]][RNA[j]][RNA[i + 1]][RNA[j - 1]][RNA[j - 2]][RNA[ip]][RNA[jp]];
		} else if (size1 == 2 && size2 == 1) {
			/* 1x2 internal loop */
			energy = param->iloop21[RNA[jp]][RNA[ip]][RNA[j - 1]][RNA[i + 2]][RNA[i + 1]][RNA[j]][RNA[i]];
		} else if (size == 2) {
			/* 1*1 internal loops */
			energy = param->iloop11[RNA[i]][RNA[i + 1]][RNA[ip]][RNA[j]][RNA[j - 1]][RNA[jp]];
		} else if ((size1 == 1 || size2 == 1) && param->gail) { 
			energy = param->tstki[fourBaseIndex(RNA[i], RNA[j], BASE_A, BASE_A)]
			         + param->tstki[fourBaseIndex(RNA[jp], RNA[ip], BASE_A, BASE_A)]
                  + param->inter[size] + loginc + param->eparam[3] + MIN(param->maxpen, 
						(lopsided * param->poppen[MIN(2, MIN(size1, size2))]));
		} else { /* General Internal loops */
			energy = param-> tstki[fourBaseIndex(RNA[i], RNA[j], RNA[i + 1], RNA[j - 1])] 
					   + param->tstki[fourBaseIndex(RNA[jp], RNA[ip], RNA[jp + 1], 
						RNA[ip - 1])] + param->inter[size] + loginc + param->eparam[3] 
					/* AM: I don't understand this eparam value, 
					I think they do not play any role currently. 
					Please look in loader.cc file, for what value 
					have been assinged to various elements of eparam array */
					//ZS: I don't understand it either and I think we should just 
					//delete it 
               + MIN(param->maxpen, (lopsided * param->poppen[MIN(2, MIN(size1, size2))]));
		}
	}
	return energy;
}

int eH(int i, int j, int* RNA, nndb_constants* param) {
	/*  Hairpin loop for all the bases between i and j */
	/*  size for size of the loop, energy is the result, 
	loginc is for the extrapolation for loops bigger than 30 */
	int size;
	int loginc;
	int energy = INFINITY_;
	int key, index, count, tlink, kmult;
	size = j - i - 1; /*  size is the number of bases in the loop, when the closing pair is excluded */

	/*  look in hairpin, and be careful that there is only 30 values */
	if (size > 30) {
		loginc = (int) (param->prelog * log(((double) size) / 30.0));
		energy = param->hairpin[30] + loginc + param->tstkh[fourBaseIndex(RNA[i], RNA[j],
				RNA[i + 1], RNA[j - 1])] + param->eparam[4]; /* size penalty + terminal mismatch stacking energy*/
	}

	else if (size <= 30 && size > 4) {
		energy = param->hairpin[size] + param->tstkh[fourBaseIndex(RNA[i], RNA[j],
				RNA[i + 1], RNA[j - 1])] + param->eparam[4]; /* size penalty + terminal mismatch stacking energy*/
	}

	else if (size == 4) {
		/*  tetraloop */
		key = 0;
		tlink = 0;
		for (index = 0; index < 6; ++index) {
			switch (RNA[i + index]) {
			case BASE_A:
				kmult = 1;
				break;
			case BASE_C:
				kmult = 2;
				break;
			case BASE_G:
				kmult = 3;
				break;
			case BASE_U:
				kmult = 4;
				break;
			default:
				kmult = 0;
				fprintf(stderr, "ERROR: in tetraloop calculation\n");
			}
			key += kmult * (int) pow(10.0, 5 - index);
		}
		/*  if the sequence is in tloop, we use this value */
		for (count = 1; count < param->numoftloops && tlink == 0; ++count) {
			if (key == param->tloop[count][0]) {
				tlink = param->tloop[count][1];
			}
		}
		energy = tlink + param->hairpin[size] + param->tstkh[fourBaseIndex(RNA[i], RNA[j],
				RNA[i + 1], RNA[j - 1])] + param->eparam[4];
	}

	else if (size == 3) {
		/*  triloop... For the moment, the file triloop.dat is empty */
		/*  else, should have a treatment like the one if size==4 */
		energy = param->hairpin[size];
		/* AM: Don't include stacking energy terms for triloopls */
		/* + tstkh[RNA[i]][RNA[j]][RNA[i+1]][RNA[j-1]]  */
		/* + eparam[4]; */
		/*  Must be another penalty for terminal AU... Not sure of this */
		energy += param->auend*auPen(RNA[i], RNA[j]);
	}

	else if (size < 3 && size != 0) {
		/*  no terminal mismatch */
		energy = param->hairpin[size] + param->eparam[4];
		if ((RNA[i] == BASE_A && RNA[j] == BASE_U) || (RNA[i] == BASE_U
				&& RNA[j] == BASE_A)) {
			energy += 6; /*  Seems to be a penalty for terminal AU.  *//* Hairpin Loops of size 3 are not allowed, the term hairpin[size] will result in a very large value.  */
		}
	} else if (size == 0)
		return INFINITY_;

	/*  GGG Bonus => GU closure preceded by GG */
	/*  i-2 = i-1 = i = G, and j = U; i < j */
	if (i > 2) {
		if (RNA[i - 2] == BASE_G && RNA[i - 1] == BASE_G && RNA[i] == BASE_G
				&& RNA[j] == BASE_U) {
			energy += param->gubonus;
			/*  printf ("\n GGG bonus for i %d j %d ", i, j); */
		}
	}

	/*  Poly-C loop => How many C are needed for being a poly-C loop */
	tlink = 1;
	for (index = 1; (index <= size) && (tlink == 1); ++index) {
		if (RNA[i + index] != BASE_C)
			tlink = 0;
	}
	if (tlink == 1) {
		if (size == 3) {
			energy += param->c3;
		} else {
			energy += param->cint + size * param->cslope;
		}
	}

	return energy;
}

int eS(int i, int j, int* RNA, nndb_constants* param) {
 	printf("RNA i is: ");
	printf("i=%d: value=%d ", i, RNA[i]);
	//ZS: Score a stack. 
   int energy;
	energy = param->stack[fourBaseIndex(RNA[i], RNA[j], RNA[i + 1], RNA[j - 1])]; 
	return energy;
}


int eM(int nr_branches, int nr_unpaired, nndb_constants* param) {
    //ZS: Score a multiloop 
    //Here, no dangling energies are taken into account, we are using the formula:
    //eM(i,j, i1, j1, ... , ik, jk) = a + b*<nr of branches> + c*<nr of unpaired nt> 
	int energy;
	int a = param->multConst[0];
	int b = param->multConst[1];
	int c = param->multConst[2];
   energy = a + nr_branches*b + nr_unpaired*c;
	return energy;
}


int eE(int* RNA, nndb_constants* param){
    //ZS: Score an external loop 
    return 0; 
}

