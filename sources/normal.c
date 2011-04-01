/** @file normal.c
 * 
 *  Mainly the routine Normalize. This routine brings terms to standard
 *	FORM. Currently it has one serious drawback. Its buffers are all
 *	in the stack. This means these buffers have a fixed size (NORMSIZE).
 *	In the past this has caused problems and NORMSIZE had to be increased.
 *
 *	It is not clear whether Normalize can be called recursively.
 */
/* #[ License : */
/*
 *   Copyright (C) 1984-2010 J.A.M. Vermaseren
 *   When using this file you are requested to refer to the publication
 *   J.A.M.Vermaseren "New features of FORM" math-ph/0010025
 *   This is considered a matter of courtesy as the development was paid
 *   for by FOM the Dutch physics granting agency and we would like to
 *   be able to track its scientific use to convince FOM of its value
 *   for the community.
 *
 *   This file is part of FORM.
 *
 *   FORM is free software: you can redistribute it and/or modify it under the
 *   terms of the GNU General Public License as published by the Free Software
 *   Foundation, either version 3 of the License, or (at your option) any later
 *   version.
 *
 *   FORM is distributed in the hope that it will be useful, but WITHOUT ANY
 *   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *   details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with FORM.  If not, see <http://www.gnu.org/licenses/>.
 */
/* #] License : */
/*
  	#[ Includes : normal.c
*/

#include "form3.h"

/*
  	#] Includes :
 	#[ Normalize :
 		#[ Commute :

	This function gets two adjacent function pointers and decides
	whether these two functions should be exchanged to obtain a
	natural ordering.

	Currently there is only an ordering of gamma matrices belonging
	to different spin lines.

*/

WORD Commute(WORD *fleft, WORD *fright)
{
	if ( *fleft >= GAMMA && *fleft <= GAMMASEVEN
	  && *fright >= GAMMA && *fright <= GAMMASEVEN ) {
		if ( fleft[FUNHEAD] < AM.OffsetIndex && fleft[FUNHEAD] > fright[FUNHEAD] )
			return(1);
	}
/*
	if other conditions will come here, keep in mind that if *fleft < 0
	or *fright < 0 they are arguments in the exponent function as in f^2
*/
	return(0);
}

/*
 		#] Commute :
 		#[ Normalize :

	This is the big normalization routine. It has a great need
	to be economical.
	There is a fixed limit to the number of objects coming in.
	Something should be done about it.

*/

WORD Normalize(PHEAD WORD *term)
{
/*
  	#[ Declarations :
*/
	GETBIDENTITY
	WORD *t, *m, *r, i, j, k, l, nsym, *ss, *tt, *u;
	WORD shortnum, stype;
	WORD *stop, *to = 0, *from = 0;
/*
	The next variables would be better off in the AT.WorkSpace (?)
	or as static global variables. Now they make stackallocations
	rather bothersome.
*/
	WORD psym[7*NORMSIZE],*ppsym;
	WORD pvec[NORMSIZE],*ppvec,nvec;
	WORD pdot[3*NORMSIZE],*ppdot,ndot;
	WORD pdel[2*NORMSIZE],*ppdel,ndel;
	WORD pind[NORMSIZE],nind;
	WORD *peps[NORMSIZE/3],neps;
	WORD *pden[NORMSIZE/3],nden;
	WORD *pcom[NORMSIZE],ncom;
	WORD *pnco[NORMSIZE],nnco;
	WORD *pcon[2*NORMSIZE],ncon;		/* Pointer to contractable indices */
	WORD ncoef;							/* Accumulator for the coefficient */
	WORD *lnum=AT.n_llnum+1,nnum;		/* Scratch for factorials */
	WORD *termout, oldtoprhs = 0, subtype;
	WORD ReplaceType, didcontr, regval = 0;
	WORD *ReplaceSub;
	WORD *fillsetexp;
	CBUF *C = cbuf+AT.ebufnum;
	WORD *ANsc = 0, *ANsm = 0, *ANsr = 0;
	LONG oldcpointer = 0;
/*
	int termflag;
*/
/*
  	#] Declarations :
  	#[ Setup :
PrintTerm(term,"Normalize");
*/

Restart:
	didcontr = 0;
	ReplaceType = -1;
	t = term;
	if ( !*t ) return(regval);
	r = t + *t;
	ncoef = r[-1];
	i = ABS(ncoef);
	r -= i;
	m = r;
	t = AT.n_coef;
	NCOPY(t,r,i);
	termout = AT.WorkPointer;
    AT.WorkPointer = (WORD *)(((UBYTE *)(AT.WorkPointer)) + AM.MaxTer);
	fillsetexp = termout+1;
	AN.PolyNormFlag = 0;
/*
	termflag = 0;
*/
/*
  	#] Setup :
  	#[ First scan :
*/
	nsym = nvec = ndot = ndel = neps = nden = 
	nind = ncom = nnco = ncon = 0;
	ppsym = psym;
	ppvec = pvec;
	ppdot = pdot;
	ppdel = pdel;
	t = term + 1;
conscan:;
	if ( t < m ) do {
		r = t + t[1];
		switch ( *t ) {
			case SYMBOL :
				t += 2;
				from = m;
				do {
					if ( t[1] == 0 ) {
/*						if ( *t == 0 || *t == MAXPOWER ) goto NormZZ; */
						t += 2;
						goto NextSymbol;
					}
					if ( *t <= DENOMINATORSYMBOL && *t >= COEFFSYMBOL ) {
/*
						if ( AN.NoScrat2 == 0 ) {
							AN.NoScrat2 = (UWORD *)Malloc1((AM.MaxTal+2)*sizeof(UWORD),"Normalize");
						}
*/
						if ( AN.cTerm ) m = AN.cTerm;
						else m = term;
						m += *m;
						ncoef = REDLENG(ncoef);
						if ( *t == COEFFSYMBOL ) {
						  i = t[1];
						  nnum = REDLENG(m[-1]);
						  m -= ABS(m[-1]);
						  if ( i > 0 ) {
							while ( i > 0 ) {
								if ( MulRat(BHEAD (UWORD *)AT.n_coef,ncoef,(UWORD *)m,nnum,
								(UWORD *)AT.n_coef,&ncoef) ) goto FromNorm;
								i--;
							}
						  }
						  else if ( i < 0 ) {
							while ( i < 0 ) {
								if ( DivRat(BHEAD (UWORD *)AT.n_coef,ncoef,(UWORD *)m,nnum,
								(UWORD *)AT.n_coef,&ncoef) ) goto FromNorm;
								i++;
							}
						  }
						}
						else {
						  i = m[-1];
						  nnum = (ABS(i)-1)/2;
						  if ( *t == NUMERATORSYMBOL ) { m -= nnum + 1; }
						  else { m--; }
						  while ( *m == 0 && nnum > 1 ) { m--; nnum--; }
						  m -= nnum;
						  if ( i < 0 && *t == NUMERATORSYMBOL ) nnum = -nnum;
						  i = t[1];
						  if ( i > 0 ) {
							while ( i > 0 ) {
								if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)m,nnum) )
										goto FromNorm;
								i--;
							}
						  }
						  else if ( i < 0 ) {
							while ( i < 0 ) {
								if ( Divvy(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)m,nnum) )
										goto FromNorm;
								i++;
							}
						  }
						}
						ncoef = INCLENG(ncoef);
						t += 2;
						goto NextSymbol;
					}
					else if ( *t == DIMENSIONSYMBOL ) {
						if ( AN.cTerm ) m = AN.cTerm;
						else m = term;
						k = DimensionTerm(BHEAD m);
						if ( k == 0 ) goto NormZero;
						if ( k == MAXPOSITIVE ) {
							LOCK(ErrorMessageLock);
							MesPrint("Dimension_ is undefined in term %t");
							UNLOCK(ErrorMessageLock);
							goto NormMin;
						}
						if ( k == -MAXPOSITIVE ) {
							LOCK(ErrorMessageLock);
							MesPrint("Dimension_ out of range in term %t");
							UNLOCK(ErrorMessageLock);
							goto NormMin;
						}
						if ( k > 0 ) { *((UWORD *)lnum) = k; nnum = 1; }
						else { *((UWORD *)lnum) = -k; nnum = -1; }
						ncoef = REDLENG(ncoef);	
						if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,nnum) ) goto FromNorm;
						ncoef = INCLENG(ncoef);
						t += 2;
						goto NextSymbol;
					}
					if ( ( *t >= MAXPOWER && *t < 2*MAXPOWER )
						|| ( *t < -MAXPOWER && *t > -2*MAXPOWER ) ) {
/*
			#[ TO SNUMBER :
*/
				if ( *t < 0 ) {
					*t += MAXPOWER;
					*t = -*t;
					if ( t[1] & 1 ) ncoef = -ncoef;
				}
				else if ( *t == MAXPOWER ) {
					if ( t[1] > 0 ) goto NormZero;
					goto NormInf;
				}
				else {
					*t -= MAXPOWER;
				}
				lnum[0] = *t;
				nnum = 1;
				if ( t[1] && RaisPow(BHEAD (UWORD *)lnum,&nnum,(UWORD)(ABS(t[1]))) )
					goto FromNorm;
				ncoef = REDLENG(ncoef);
				if ( t[1] < 0 ) {
					if ( Divvy(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,nnum) )
						goto FromNorm;
				}
				else if ( t[1] > 0 ) {
					if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,nnum) )
						goto FromNorm;
				}
				ncoef = INCLENG(ncoef);
/*
			#] TO SNUMBER :
*/
						t += 2;
						goto NextSymbol;
					}
					if ( ( *t <= NumSymbols && *t > -MAXPOWER )
					 && ( symbols[*t].complex & VARTYPEROOTOFUNITY ) == VARTYPEROOTOFUNITY ) {
						if ( t[1] <= 2*MAXPOWER && t[1] >= -2*MAXPOWER ) {
						t[1] %= symbols[*t].maxpower;
						if ( t[1] < 0 ) t[1] += symbols[*t].maxpower;
						if ( ( symbols[*t].complex & VARTYPEMINUS ) == VARTYPEMINUS ) {
							if ( ( ( symbols[*t].maxpower & 1 ) == 0 ) &&
							( t[1] >= symbols[*t].maxpower/2 ) ) {
								t[1] -= symbols[*t].maxpower/2; ncoef = -ncoef;
							}
						}
						if ( t[1] == 0 ) { t += 2; goto NextSymbol; }
						}
					}
					i = nsym;
					m = ppsym;
					if ( i > 0 ) do {
						m -= 2;
						if	( *t == *m ) {
							t++; m++;
							if	( *t > 2*MAXPOWER || *t < -2*MAXPOWER
							||	*m > 2*MAXPOWER || *m < -2*MAXPOWER ) {
								LOCK(ErrorMessageLock);
								MesPrint("Illegal wildcard power combination.");
								UNLOCK(ErrorMessageLock);
								goto NormMin;
							}
							*m += *t;
							if ( ( t[-1] <= NumSymbols && t[-1] > -MAXPOWER )
							 && ( symbols[t[-1]].complex & VARTYPEROOTOFUNITY ) == VARTYPEROOTOFUNITY ) {
								*m %= symbols[t[-1]].maxpower;
								if ( *m < 0 ) *m += symbols[t[-1]].maxpower;
								if ( ( symbols[t[-1]].complex & VARTYPEMINUS ) == VARTYPEMINUS ) {
									if ( ( ( symbols[t[-1]].maxpower & 1 ) == 0 ) &&
									( *m >= symbols[t[-1]].maxpower/2 ) ) {
										*m -= symbols[t[-1]].maxpower/2; ncoef = -ncoef;
									}
								}
							}
							if	( *m >= 2*MAXPOWER || *m <= -2*MAXPOWER ) {
								LOCK(ErrorMessageLock);
								MesPrint("Power overflow during normalization");
								UNLOCK(ErrorMessageLock);
								goto NormMin;
							}
							if ( !*m ) {
								m--;
								while ( i < nsym )
									{ *m = m[2]; m++; *m = m[2]; m++; i++; }
								ppsym -= 2;
								nsym--;
							}
							t++;
							goto NextSymbol;
						}
					} while ( *t < *m && --i > 0 );
					m = ppsym;
					while ( i < nsym )
						{ m--; m[2] = *m; m--; m[2] = *m; i++; }
					*m++ = *t++;
					*m = *t++;
					ppsym += 2;
					nsym++;
NextSymbol:;
				} while ( t < r );
				m = from;
				break;
			case VECTOR :
				t += 2;
				do {
					if ( t[1] == FUNNYVEC ) {
						pind[nind++] = *t;
						t += 2;
					}
					else if ( t[1] < 0 ) {
						if ( *t == NOINDEX && t[1] == NOINDEX ) t += 2;
						else {
						*ppdot++ = *t++; *ppdot++ = *t++; *ppdot++ = 1; ndot++; 
						}
					}
					else { *ppvec++ = *t++; *ppvec++ = *t++; nvec += 2; }
				} while ( t < r );
				break;
			case DOTPRODUCT :
				t += 2;
				do {
					if ( t[2] == 0 ) t += 3;
					else if ( ndot > 0 && t[0] == ppdot[-3]
						&& t[1] == ppdot[-2] ) {
						ppdot[-1] += t[2];
						t += 3;
						if ( ppdot[-1] == 0 ) { ppdot -= 3; ndot--; }
					}
					else {
						*ppdot++ = *t++; *ppdot++ = *t++;
						*ppdot++ = *t++; ndot++;
					}
				} while ( t < r );
				break;
			case HAAKJE :
				break;
			case SETSET:
				if ( WildFill(BHEAD termout,term,AT.dummysubexp) < 0 ) goto FromNorm;
				i = *termout;
				t = termout; m = term;
				NCOPY(m,t,i);
				goto Restart;
			case DOLLAREXPRESSION :
/*
				We have DOLLAREXPRESSION,4,number,power
				Replace by SUBEXPRESSION and exit elegantly to let
				TestSub pick it up. Of course look for special cases first.
				Note that we have a special compiler buffer for the values.
*/
				if ( AR.Eside != LHSIDE ) {
				DOLLARS d = Dollars + t[2];
#ifdef WITHPTHREADS
				int nummodopt, ptype = -1;
				if ( AS.MultiThreaded ) {
					for ( nummodopt = 0; nummodopt < NumModOptdollars; nummodopt++ ) {
						if ( t[2] == ModOptdollars[nummodopt].number ) break;
					}
					if ( nummodopt < NumModOptdollars ) {
						ptype = ModOptdollars[nummodopt].type;
						if ( ptype == MODLOCAL ) {
							d = ModOptdollars[nummodopt].dstruct+AT.identity;
						}
						else {
							LOCK(d->pthreadslockread);
						}
					}
				}
#endif
				if ( d->type == DOLZERO ) {
#ifdef WITHPTHREADS
					if ( ptype > 0 && ptype != MODLOCAL ) { UNLOCK(d->pthreadslockread); }
#endif
					if ( t[3] == 0 ) goto NormZZ;
					if ( t[3] < 0 ) goto NormInf;
					goto NormZero;
				}
				else if ( d->type == DOLNUMBER ) {
					nnum = d->where[0];
					if ( nnum > 0 ) {
						nnum = d->where[nnum-1];
						if ( nnum < 0 ) { ncoef = -ncoef; nnum = -nnum; }
						nnum = (nnum-1)/2;
						for ( i = 1; i <= nnum; i++ ) lnum[i-1] = d->where[i];
					}
					if ( nnum == 0 || ( nnum == 1 && lnum[0] == 0 ) ) {
#ifdef WITHPTHREADS
						if ( ptype > 0 && ptype != MODLOCAL ) { UNLOCK(d->pthreadslockread); }
#endif
						if ( t[3] < 0 ) goto NormInf;
						else if ( t[3] == 0 ) goto NormZZ;
						goto NormZero;
					}
					if ( t[3] && RaisPow(BHEAD (UWORD *)lnum,&nnum,(UWORD)(ABS(t[3]))) ) goto FromNorm;
					ncoef = REDLENG(ncoef);
					if ( t[3] < 0 ) {
						if ( Divvy(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,nnum) ) {
#ifdef WITHPTHREADS
							if ( ptype > 0 && ptype != MODLOCAL ) { UNLOCK(d->pthreadslockread); }
#endif
							goto FromNorm;
						}
					}
					else if ( t[3] > 0 ) {
						if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,nnum) ) {
#ifdef WITHPTHREADS
							if ( ptype > 0 && ptype != MODLOCAL ) { UNLOCK(d->pthreadslockread); }
#endif
							goto FromNorm;
						}
					}
					ncoef = INCLENG(ncoef);
				}
				else if ( d->type == DOLINDEX ) {
					if ( d->index == 0 ) {
#ifdef WITHPTHREADS
						if ( ptype > 0 && ptype != MODLOCAL ) { UNLOCK(d->pthreadslockread); }
#endif
						goto NormZero;
					}
					if ( d->index != NOINDEX ) pind[nind++] = d->index;
				}
				else if ( d->type == DOLTERMS ) {
					t[0] = SUBEXPRESSION;
					t[4] = AM.dbufnum;
					if ( t[3] == 0 ) {
#ifdef WITHPTHREADS
						if ( ptype > 0 && ptype != MODLOCAL ) { UNLOCK(d->pthreadslockread); }
#endif
						break;
					}
					regval = 2;
					t = r;
					while ( t < m ) {
						if ( *t == DOLLAREXPRESSION ) {
#ifdef WITHPTHREADS
							if ( ptype > 0 && ptype != MODLOCAL ) { UNLOCK(d->pthreadslockread); }
#endif
							d = Dollars + t[2];
#ifdef WITHPTHREADS
							if ( AS.MultiThreaded ) {
								for ( nummodopt = 0; nummodopt < NumModOptdollars; nummodopt++ ) {
									if ( t[2] == ModOptdollars[nummodopt].number ) break;
								}
								if ( nummodopt < NumModOptdollars ) {
									ptype = ModOptdollars[nummodopt].type;
									if ( ptype == MODLOCAL ) {
										d = ModOptdollars[nummodopt].dstruct+AT.identity;
									}
									else {
										LOCK(d->pthreadslockread);
									}
								}
							}
#endif
							if ( d->type == DOLTERMS ) {
								t[0] = SUBEXPRESSION;
								t[4] = AM.dbufnum;
							}
						}
						t += t[1];
					}
#ifdef WITHPTHREADS
					if ( ptype > 0 && ptype != MODLOCAL ) { UNLOCK(d->pthreadslockread); }
#endif
					goto RegEnd;
				}
				else {
#ifdef WITHPTHREADS
					if ( ptype > 0 && ptype != MODLOCAL ) { UNLOCK(d->pthreadslockread); }
#endif
					LOCK(ErrorMessageLock);
					MesPrint("!!!This $ variation has not been implemented yet!!!");
					UNLOCK(ErrorMessageLock);
					goto NormMin;
				}
#ifdef WITHPTHREADS
				if ( ptype > 0 && ptype != MODLOCAL ) { UNLOCK(d->pthreadslockread); }
#endif
				}
				else {
					pnco[nnco++] = t;
/*
					The next statement should be safe as the value is used
					only by the compiler (ie the master).
*/
					AC.lhdollarflag = 1;
				}
				break;
			case DELTA :
				t += 2;
				do {
					if ( *t < 0 ) {
						if ( *t == SUMMEDIND ) {
							if ( t[1] < -NMIN4SHIFT ) {
								k = -t[1]-NMIN4SHIFT;
								k = ExtraSymbol(k,1,nsym,ppsym,&ncoef);
								nsym += k;
								ppsym += (k << 1);
							}
							else if ( t[1] == 0 ) goto NormZero;
							else {
								if ( t[1] < 0 ) {
									lnum[0] = -t[1];
									nnum = -1;
								}
								else {
									lnum[0] = t[1];
									nnum = 1;
								}
								ncoef = REDLENG(ncoef);
								if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,nnum) )
									goto FromNorm;
								ncoef = INCLENG(ncoef);
							}
							t += 2;
						}
						else if ( *t == NOINDEX && t[1] == NOINDEX ) t += 2;
						else if ( *t == EMPTYINDEX && t[1] == EMPTYINDEX ) {
							*ppdel++ = *t++; *ppdel++ = *t++; ndel += 2;
						}
						else
						if ( t[1] < 0 ) {
							*ppdot++ = *t++; *ppdot++ = *t++; *ppdot++ = 1; ndot++; 
						}
						else {
							*ppvec++ = *t++; *ppvec++ = *t++; nvec += 2;
						}
					}
					else {
						if ( t[1] < 0 ) {
							*ppvec++ = t[1]; *ppvec++ = *t; t+=2; nvec += 2;
						}
						else { *ppdel++ = *t++; *ppdel++ = *t++; ndel += 2; }
					}
				} while ( t < r );
				break;
			case FACTORIAL :
/*
				(FACTORIAL,FUNHEAD+2,..,-SNUMBER,number)
*/
				if ( t[FUNHEAD] == -SNUMBER && t[1] == FUNHEAD+2
											&& t[FUNHEAD+1] >= 0 ) {
					if ( Factorial(BHEAD t[FUNHEAD+1],(UWORD *)lnum,&nnum) )
						goto FromNorm;
MulIn:
					ncoef = REDLENG(ncoef);	
					if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,nnum) ) goto FromNorm;
					ncoef = INCLENG(ncoef);
				}
				else pcom[ncom++] = t;
				break;
			case BERNOULLIFUNCTION :
/*
				(BERNOULLIFUNCTION,FUNHEAD+2,..,-SNUMBER,number)
*/
				if ( ( t[FUNHEAD] == -SNUMBER && t[FUNHEAD+1] >= 0 )
				&& ( t[1] == FUNHEAD+2 || ( t[1] == FUNHEAD+4 &&
				t[FUNHEAD+2] == -SNUMBER && ABS(t[FUNHEAD+3]) == 1 ) ) ) {
					WORD inum, mnum;
					if ( Bernoulli(t[FUNHEAD+1],(UWORD *)lnum,&nnum) )
						goto FromNorm;
					if ( nnum == 0 ) goto NormZero;
					inum = nnum; if ( inum < 0 ) inum = -inum;
					inum--; inum /= 2;
					mnum = inum;
					while ( lnum[mnum-1] == 0 ) mnum--;
					if ( nnum < 0 ) mnum = -mnum;
					ncoef = REDLENG(ncoef);	
					if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,mnum) ) goto FromNorm;
					mnum = inum;
					while ( lnum[inum+mnum-1] == 0 ) mnum--;
					if ( Divvy(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)(lnum+inum),mnum) ) goto FromNorm;
					ncoef = INCLENG(ncoef);
					if ( t[1] == FUNHEAD+4 && t[FUNHEAD+1] == 1
					 && t[FUNHEAD+3] == -1 ) ncoef = -ncoef; 
				}
				else pcom[ncom++] = t;
				break;
			case NUMARGSFUN:
/*
				Numerical function giving the number of arguments.
*/
				k = 0;
				t += FUNHEAD;
				while ( t < r ) {
					k++;
					NEXTARG(t);
				}
				if ( k == 0 ) goto NormZero;
				*((UWORD *)lnum) = k;
				nnum = 1;
				goto MulIn;
			case NUMTERMSFUN:
/*
				Numerical function giving the number of terms in the single argument.
*/
				if ( t[FUNHEAD] < 0 ) {
					if ( t[FUNHEAD] <= -FUNCTION && t[1] == FUNHEAD+1 ) break;
					if ( t[FUNHEAD] > -FUNCTION && t[1] == FUNHEAD+2 ) break;
					pcom[ncom++] = t;
					break;
				}
				if ( t[FUNHEAD] > 0 && t[FUNHEAD] == t[1]-FUNHEAD ) {
					k = 0;
					t += FUNHEAD+ARGHEAD;
					while ( t < r ) {
						k++;
						t += *t;
					}
					if ( k == 0 ) goto NormZero;
					*((UWORD *)lnum) = k;
					nnum = 1;
					goto MulIn;
				}
				else pcom[ncom++] = t;
				break;
			case COUNTFUNCTION:
				if ( AN.cTerm ) {
					k = CountFun(AN.cTerm,t);
					if ( k == 0 ) goto NormZero;
					if ( k > 0 ) { *((UWORD *)lnum) = k; nnum = 1; }
					else { *((UWORD *)lnum) = -k; nnum = -1; }
					goto MulIn;
				}
				break;
			case TERMFUNCTION:
				if ( AN.cTerm ) {
					ANsr = r; ANsm = m; ANsc = AN.cTerm;
					AN.cTerm = 0;
					t = ANsc + 1;
					m = ANsc + *ANsc;
					ncoef = REDLENG(ncoef);
					nnum = REDLENG(m[-1]);	
					m -= ABS(m[-1]);
					if ( MulRat(BHEAD (UWORD *)AT.n_coef,ncoef,(UWORD *)m,nnum,
					(UWORD *)AT.n_coef,&ncoef) ) goto FromNorm;
					ncoef = INCLENG(ncoef);
					r = t;
				}
				break;
			case FIRSTBRACKET:
				if ( ( t[1] == FUNHEAD+2 ) && t[FUNHEAD] == -EXPRESSION ) {
					if ( GetFirstBracket(termout,t[FUNHEAD+1]) < 0 ) goto FromNorm;
					if ( *termout > 4 ) {
						WORD *r1, *r2, *r3;
						while ( r < m ) *t++ = *r++;
						r1 = term + *term;
						r2 = termout + *termout; r2 -= ABS(r2[-1]);
						while ( r < r1 ) *r2++ = *r++;
						r3 = termout + 1;
						while ( r3 < r2 ) *t++ = *r3++; *term = t - term;
						if ( AT.WorkPointer > term && AT.WorkPointer < t )
							AT.WorkPointer = t;
						goto Restart;
					}
				}
				break;
			case TERMSINEXPR:
				{ LONG x;
				if ( ( t[1] == FUNHEAD+2 ) && t[FUNHEAD] == -EXPRESSION ) {
					x = TermsInExpression(t[FUNHEAD+1]);
multermnum:			if ( x == 0 ) goto NormZero;
					if ( x < 0 ) {
						x = -x;
						if ( x > WORDMASK ) { lnum[0] = x & WORDMASK;
							lnum[1] = x >> BITSINWORD; nnum = -2;
						}
						else { lnum[0] = x; nnum = -1; }
					}
					else if ( x > WORDMASK ) {
						lnum[0] = x & WORDMASK;
						lnum[1] = x >> BITSINWORD;
						nnum = 2;
					}
					else { lnum[0] = x; nnum = 1; }
					ncoef = REDLENG(ncoef);
					if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,nnum) )
						goto FromNorm;
					ncoef = INCLENG(ncoef);
				}
				else if ( ( t[1] == FUNHEAD+2 ) && t[FUNHEAD] == -DOLLAREXPRESSION ) {
					x = TermsInDollar(t[FUNHEAD+1]);
					goto multermnum;
				}
				else { pcom[ncom++] = t; }
				}
				break;
			case MATCHFUNCTION:
			case PATTERNFUNCTION:
				break;
			case BINOMIAL:
/*
				Binomial function for internal use for the moment.
				The routine in reken.c should be more efficient.
*/
				if ( t[1] == FUNHEAD+4 && t[FUNHEAD] == -SNUMBER
				&& t[FUNHEAD+1] >= 0 && t[FUNHEAD+2] == -SNUMBER
				&& t[FUNHEAD+3] >= 0 && t[FUNHEAD+1] >= t[FUNHEAD+3] ) {
					if ( t[FUNHEAD+1] > t[FUNHEAD+3] ) {
						if ( GetBinom((UWORD *)lnum,&nnum,
						t[FUNHEAD+1],t[FUNHEAD+3]) ) goto FromNorm;
						if ( nnum == 0 ) goto NormZero;
						goto MulIn;
					}
				}
				else pcom[ncom++] = t;
				break;
			case SIGNFUN:
/*
				Numerical function giving (-1)^arg
*/
				if ( t[1] == FUNHEAD+2 && t[FUNHEAD] == -SNUMBER ) {
					if ( ( t[FUNHEAD+1] & 1 ) != 0 ) ncoef = -ncoef;
				}
				else if ( ( t[FUNHEAD] > 0 ) && ( t[1] == FUNHEAD+t[FUNHEAD] )
				&& ( t[FUNHEAD] == ARGHEAD+1+abs(t[t[1]-1]) ) ) {
					UWORD *numer1,*denom1;
					WORD nsize = abs(t[t[1]-1]), nnsize, isize;
					nnsize = (nsize-1)/2;
					numer1 = (UWORD *)(t + FUNHEAD+ARGHEAD+1);
					denom1 = numer1 + nnsize;
					for ( isize = 1; isize < nnsize; isize++ ) {
						if ( denom1[isize] ) break;
					}
					if ( ( denom1[0] != 1 ) || isize < nnsize ) {
						pcom[ncom++] = t;
					}
					else {
						if ( ( numer1[0] & 1 ) != 0 ) ncoef = -ncoef;
					}
				}
				else {
					goto doflags;
/*					pcom[ncom++] = t; */
				}
				break;
			case SIGFUNCTION:
/*
				Numerical function giving the sign of the numerical argument
				The sign of zero is 1.
				If there are roots of unity they are part of the sign.
*/
				if ( t[1] == FUNHEAD+2 && t[FUNHEAD] == -SNUMBER ) {
					if ( t[FUNHEAD+1] < 0 ) ncoef = -ncoef;
				}
				else if ( ( t[1] == FUNHEAD+2 ) && ( t[FUNHEAD] == -SYMBOL )
					&& ( ( t[FUNHEAD+1] <= NumSymbols && t[FUNHEAD+1] > -MAXPOWER )
					&& ( symbols[t[FUNHEAD+1]].complex & VARTYPEROOTOFUNITY ) == VARTYPEROOTOFUNITY ) ) {
					k = t[FUNHEAD+1];
					from = m;
					i = nsym;
					m = ppsym;
					if ( i > 0 ) do {
						m -= 2;
						if	( k == *m ) {
							m++;
							*m = *m + 1;
							*m %= symbols[k].maxpower;
							if ( ( symbols[k].complex & VARTYPEMINUS ) == VARTYPEMINUS ) {
								if ( ( ( symbols[k].maxpower & 1 ) == 0 ) &&
								( *m >= symbols[k].maxpower/2 ) ) {
									*m -= symbols[k].maxpower/2; ncoef = -ncoef;
								}
							}
							if ( !*m ) {
								m--;
								while ( i < nsym )
									{ *m = m[2]; m++; *m = m[2]; m++; i++; }
								ppsym -= 2;
								nsym--;
							}
							goto sigDoneSymbol;
						}
					} while ( k < *m && --i > 0 );
					m = ppsym;
					while ( i < nsym )
						{ m--; m[2] = *m; m--; m[2] = *m; i++; }
					*m++ = k;
					*m = 1;
					ppsym += 2;
					nsym++;
sigDoneSymbol:;
					m = from;
				}
				else if ( ( t[FUNHEAD] > 0 ) && ( t[1] == FUNHEAD+t[FUNHEAD] ) ) {
					if ( t[FUNHEAD] == ARGHEAD+1+abs(t[t[1]-1]) ) {
						if ( t[t[1]-1] < 0 ) ncoef = -ncoef;
					}
/*
					Now we should fish out the roots of unity
*/
					else if ( ( t[FUNHEAD+ARGHEAD]+FUNHEAD+ARGHEAD == t[1] )
					&& ( t[FUNHEAD+ARGHEAD+1] == SYMBOL ) ) {
						WORD *ts = t + FUNHEAD+ARGHEAD+3;
						WORD its = ts[-1]-2;
						while ( its > 0 ) {
							if ( ( *ts != 0 ) && ( ( *ts > NumSymbols || *ts <= -MAXPOWER )
							 || ( symbols[*ts].complex & VARTYPEROOTOFUNITY ) != VARTYPEROOTOFUNITY ) ) {
								goto signogood;
							}
							ts += 2; its -= 2;
						}
/*
						Now we have only roots of unity which should be
						registered in the list of sysmbols.
*/
						if ( t[t[1]-1] < 0 ) ncoef = -ncoef;
						ts = t + FUNHEAD+ARGHEAD+3;
						its = ts[-1]-2;
						from = m;
						while ( its > 0 ) {
							i = nsym;
							m = ppsym;
							if ( i > 0 ) do {
								m -= 2;
								if	( *ts == *m ) {
									ts++; m++;
									*m += *ts;
									if ( ( ts[-1] <= NumSymbols && ts[-1] > -MAXPOWER ) &&
									 ( symbols[ts[-1]].complex & VARTYPEROOTOFUNITY ) == VARTYPEROOTOFUNITY ) {
										*m %= symbols[ts[-1]].maxpower;
										if ( *m < 0 ) *m += symbols[ts[-1]].maxpower;
										if ( ( symbols[ts[-1]].complex & VARTYPEMINUS ) == VARTYPEMINUS ) {
											if ( ( ( symbols[ts[-1]].maxpower & 1 ) == 0 ) &&
											( *m >= symbols[ts[-1]].maxpower/2 ) ) {
												*m -= symbols[ts[-1]].maxpower/2; ncoef = -ncoef;
											}
										}
									}
									if ( !*m ) {
										m--;
										while ( i < nsym )
											{ *m = m[2]; m++; *m = m[2]; m++; i++; }
										ppsym -= 2;
										nsym--;
									}
									ts++; its -= 2;
									goto sigNextSymbol;
								}
							} while ( *ts < *m && --i > 0 );
							m = ppsym;
							while ( i < nsym )
								{ m--; m[2] = *m; m--; m[2] = *m; i++; }
							*m++ = *ts++;
							*m = *ts++;
							ppsym += 2;
							nsym++; its -= 2;
sigNextSymbol:;
						}
						m = from;
					}
					else {
signogood:				pcom[ncom++] = t;
					}
				}
				else pcom[ncom++] = t;
				break;
			case ABSFUNCTION:
/*
				Numerical function giving the absolute value of the
				numerical argument. Or roots of unity.
*/
				if ( t[1] == FUNHEAD+2 && t[FUNHEAD] == -SNUMBER ) {
					k = t[FUNHEAD+1];
					if ( k < 0 ) k = -k;
					if ( k == 0 ) goto NormZero;
					*((UWORD *)lnum) = k; nnum = 1;
					goto MulIn;

				}
				else if ( t[1] == FUNHEAD+2 && t[FUNHEAD] == -SYMBOL ) {
					k = t[FUNHEAD+1];
					if ( ( k > NumSymbols || k <= -MAXPOWER )
					 || ( symbols[k].complex & VARTYPEROOTOFUNITY ) != VARTYPEROOTOFUNITY )
						goto absnogood;
				}
				else if ( ( t[FUNHEAD] > 0 ) && ( t[1] == FUNHEAD+t[FUNHEAD] )
				&& ( t[1] == FUNHEAD+ARGHEAD+t[FUNHEAD+ARGHEAD] ) ) {
					if ( t[FUNHEAD] == ARGHEAD+1+abs(t[t[1]-1]) ) {
						WORD *ts;
absnosymbols:			ts = t + t[1] -1;
						ncoef = REDLENG(ncoef);
						nnum = REDLENG(*ts);	
						if ( nnum < 0 ) nnum = -nnum;
						if ( MulRat(BHEAD (UWORD *)AT.n_coef,ncoef,
						(UWORD *)(ts-ABS(*ts)+1),nnum,
						(UWORD *)AT.n_coef,&ncoef) ) goto FromNorm;
						ncoef = INCLENG(ncoef);
					}
/*
					Now get rid of the roots of unity. This includes i_
*/
					else if ( t[FUNHEAD+ARGHEAD+1] == SYMBOL ) {
						WORD *ts = t+FUNHEAD+ARGHEAD+1;
						WORD its = ts[1] - 2;
						ts += 2;
						while ( its > 0 ) {
							if ( *ts == 0 ) { }
							else if ( ( *ts > NumSymbols || *ts <= -MAXPOWER )
							 || ( symbols[*ts].complex & VARTYPEROOTOFUNITY )
								!= VARTYPEROOTOFUNITY ) goto absnogood;
							its -= 2; ts += 2;
						}
						goto absnosymbols;
					}
					else {
absnogood:					pcom[ncom++] = t;
					}
				}
				else pcom[ncom++] = t;
				break;
			case MODFUNCTION:
			case MOD2FUNCTION:
/*
				Mod function. Does work if two arguments and the
				second argument is a positive short number
*/
				if ( t[1] == FUNHEAD+4 && t[FUNHEAD] == -SNUMBER
					&& t[FUNHEAD+2] == -SNUMBER && t[FUNHEAD+3] > 1 ) {
					WORD tmod;
					tmod = t[FUNHEAD+1]%t[FUNHEAD+3];
					if ( tmod < 0 ) tmod += t[FUNHEAD+3];
					*((UWORD *)lnum) = tmod;
					if ( *lnum == 0 ) goto NormZero;
					nnum = 1;
					goto MulIn;
				}
				else if ( t[1] > t[FUNHEAD+2] && t[FUNHEAD] > 0
				&& t[FUNHEAD+t[FUNHEAD]] == -SNUMBER
				&& t[FUNHEAD+t[FUNHEAD]+1] > 1
				&& t[1] == FUNHEAD+2+t[FUNHEAD] ) {
					WORD *ttt = t+FUNHEAD, iii;
					iii = ttt[*ttt-1];
					if ( *ttt == ttt[ARGHEAD]+ARGHEAD &&
						ttt[ARGHEAD] == ABS(iii)+1 ) {
						WORD ncmod = 1;
						WORD cmod = ttt[*ttt+1];
						iii = REDLENG(iii);
						if ( *t == MODFUNCTION ) {
							if ( TakeModulus((UWORD *)(ttt+ARGHEAD+1)
							,&iii,&cmod,ncmod,UNPACK|NOINVERSES) )
								goto FromNorm;
						}
						else {
							if ( TakeModulus((UWORD *)(ttt+ARGHEAD+1)
							,&iii,&cmod,ncmod,UNPACK|POSNEG|NOINVERSES) )
								goto FromNorm;
						}
						*((UWORD *)lnum) = ttt[ARGHEAD+1];
						if ( *lnum == 0 ) goto NormZero;
						nnum = 1;
						goto MulIn;
					}						
				}
				else if ( t[1] == FUNHEAD+2 && t[FUNHEAD] == -SNUMBER ) {
					*((UWORD *)lnum) = t[FUNHEAD+1];
					if ( *lnum == 0 ) goto NormZero;
					nnum = 1;
					goto MulIn;
				}
				else if ( ( ( t[FUNHEAD] < 0 ) && ( t[FUNHEAD] == -SNUMBER )
				&& ( t[1] >= ( FUNHEAD+6+ARGHEAD ) )
				&& ( t[FUNHEAD+2] >= 4+ARGHEAD )
				&& ( t[t[1]-1] == t[FUNHEAD+2+ARGHEAD]-1 ) ) ||
				( ( t[FUNHEAD] > 0 )
				&& ( t[FUNHEAD]-ARGHEAD-1 == ABS(t[FUNHEAD+t[FUNHEAD]-1]) )
				&& ( t[FUNHEAD+t[FUNHEAD]]-ARGHEAD-1 == t[t[1]-1] ) ) ) {
/*
					Check that the last (long) number is integer
*/
					WORD *ttt = t + t[1], iii, iii1;
					UWORD coefbuf[2], *coef2, ncoef2;
					iii = (ttt[-1]-1)/2;
					ttt -= iii;
					if ( ttt[-1] != 1 ) {
exitfromhere:
						pcom[ncom++] = t;
						break;
					}
					iii--;
					for ( iii1 = 0; iii1 < iii; iii1++ ) {
						if ( ttt[iii1] != 0 ) goto exitfromhere;
					}
/*
					Now we have a hit!
					The first argument will be put in lnum.
					It will be a rational.
					The second argument will be a long integer in coef2.
*/
					ttt = t + FUNHEAD;
					if ( *ttt < 0 ) {
						if ( ttt[1] < 0 ) {
							nnum = -1; lnum[0] = -ttt[1]; lnum[1] = 1;
						}
						else {
							nnum = 1; lnum[0] = ttt[1]; lnum[1] = 1;
						}
					}
					else {
						nnum = ABS(ttt[ttt[0]-1] - 1);
						for ( iii = 0; iii < nnum; iii++ ) {
							lnum[iii] = ttt[ARGHEAD+1+iii];
						}
						nnum = nnum/2;
						if ( ttt[ttt[0]-1] < 0 ) nnum = -nnum;
					}
					NEXTARG(ttt);
					if ( *ttt < 0 ) {
						coef2 = coefbuf;
						ncoef2 = 3; *coef2 = (UWORD)(ttt[1]);
						coef2[1] = 1;
					}
					else {
						coef2 = (UWORD *)(ttt+ARGHEAD+1);
						ncoef2 = (ttt[ttt[0]-1]-1)/2;
					}
					if ( TakeModulus((UWORD *)lnum,&nnum,(WORD *)coef2,ncoef2,
									UNPACK|NOINVERSES|FROMFUNCTION) ) {
						goto FromNorm;
					}
/*
					Do we have to pack? No, because the answer is not a fraction
*/
					ncoef = REDLENG(ncoef);
					if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,nnum) )
							goto FromNorm;
					ncoef = INCLENG(ncoef);
				}
				else pcom[ncom++] = t;
				break;
			case GCDFUNCTION:
#ifdef NEWGCDFUNCTION
				{
/*
				Has two integer arguments
				Four cases: S,S, S,L, L,S, L,L
*/
				WORD *num1, *num2, size1, size2, stor1, stor2, *ttt, ti;
				if ( t[1] == FUNHEAD+4 && t[FUNHEAD] == -SNUMBER
				&& t[FUNHEAD+2] == -SNUMBER && t[FUNHEAD+1] != 0
				&& t[FUNHEAD+3] != 0 ) {	/* Short,Short */
					stor1 = t[FUNHEAD+1];
					stor2 = t[FUNHEAD+3];
					if ( stor1 < 0 ) stor1 = -stor1;
					if ( stor2 < 0 ) stor2 = -stor2;
					num1 = &stor1; num2 = &stor2;
					size1 = size2 = 1;
					goto gcdcalc;
				}
				else if ( t[1] > FUNHEAD+4 ) {
					if ( t[FUNHEAD] == -SNUMBER && t[FUNHEAD+1] != 0
					&& t[FUNHEAD+2] == t[1]-FUNHEAD-2 &&
					ABS(t[t[1]-1]) == t[FUNHEAD+2]-1-ARGHEAD ) { /* Short,Long */
						num2 = t + t[1];
						size2 = ABS(num2[-1]);
						ttt = num2-1;
						num2 -= size2;
						size2 = (size2-1)/2;
						ti = size2;
						while ( ti > 1 && ttt[-1] == 0 ) { ttt--; ti--; }
						if ( ti == 1 && ttt[-1] == 1 ) {
							stor1 = t[FUNHEAD+1];
							if ( stor1 < 0 ) stor1 = -stor1;
							num1 = &stor1;
							size1 = 1;
							goto gcdcalc;
						}
						else pcom[ncom++] = t;
					}
					else if ( t[FUNHEAD] > 0 &&
					t[FUNHEAD]-1-ARGHEAD == ABS(t[t[FUNHEAD]+FUNHEAD-1]) ) {
						num1 = t + FUNHEAD + t[FUNHEAD];
						size1 = ABS(num1[-1]);
						ttt = num1-1;
						num1 -= size1;
						size1 = (size1-1)/2;
						ti = size1;
						while ( ti > 1 && ttt[-1] == 0 ) { ttt--; ti--; }
						if ( ti == 1 && ttt[-1] == 1 ) {
						 if ( t[1]-FUNHEAD == t[FUNHEAD]+2 && t[t[1]-2] == -SNUMBER
						 && t[t[1]-1] != 0 ) { /* Long,Short */
							stor2 = t[t[1]-1];
							if ( stor2 < 0 ) stor2 = -stor2;
							num2 = &stor2;
							size2 = 1;
							goto gcdcalc;
						 }
						 else if ( t[1]-FUNHEAD == t[FUNHEAD]+t[FUNHEAD+t[FUNHEAD]]
						 && ABS(t[t[1]-1]) == t[FUNHEAD+t[FUNHEAD]] - ARGHEAD-1 ) {
						  num2 = t + t[1];
						  size2 = ABS(num2[-1]);
						  ttt = num2-1;
						  num2 -= size2;
						  size2 = (size2-1)/2;
						  ti = size2;
						  while ( ti > 1 && ttt[-1] == 0 ) { ttt--; ti--; }
						  if ( ti == 1 && ttt[-1] == 1 ) {
gcdcalc:					if ( GcdLong(BHEAD (UWORD *)num1,size1,(UWORD *)num2,size2
								,(UWORD *)lnum,&nnum) ) goto FromNorm;
							goto MulIn;
						  }
						  else pcom[ncom++] = t;
						 }
						 else pcom[ncom++] = t;
						}
						else pcom[ncom++] = t;
					}
					else pcom[ncom++] = t;
				}
				else pcom[ncom++] = t;
				}
#else
				{
					WORD *gcd = AT.WorkPointer;
					if ( ( gcd = EvaluateGcd(BHEAD t) ) == 0 ) goto FromNorm;
					if ( *gcd == 4 && gcd[1] == 1 && gcd[2] == 1 && gcd[4] == 0 ) {
						AT.WorkPointer = gcd;
					}
					else if ( gcd[*gcd] == 0 ) {
						WORD *t1, iii, change, *num, *den, numsize, densize;
						if ( gcd[*gcd-1] < *gcd-1 ) {
							t1 = gcd+1;
							for ( iii = 2; iii < t1[1]; iii += 2 ) {
								change = ExtraSymbol(t1[iii],t1[iii+1],nsym,ppsym,&ncoef);
								nsym += change;
								ppsym += change << 1;
							}
						}
						t1 = gcd + *gcd;
						iii = t1[-1]; num = t1-iii; numsize = (iii-1)/2;
						den = num + numsize; densize = numsize;
						while ( numsize > 1 && num[numsize-1] == 0 ) numsize--;
						while ( densize > 1 && den[densize-1] == 0 ) densize--;
						if ( numsize > 1 || num[0] != 1 ) {
							ncoef = REDLENG(ncoef);
							if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)num,numsize) ) goto FromNorm;
							ncoef = INCLENG(ncoef);
						}
						if ( densize > 1 || den[0] != 1 ) {
							ncoef = REDLENG(ncoef);
							if ( Divvy(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)den,densize) ) goto FromNorm;
							ncoef = INCLENG(ncoef);
						}
						AT.WorkPointer = gcd;
					}
					else {	/* a whole expression */
/*
						Action: Put the expression in a compiler buffer.
						Insert a SUBEXPRESSION subterm
						Set the return value of the routine such that in
						Generator the term gets sent again to TestSub.

						1: put in C (ebufnum)
						2: after that the WorkSpace is free again.
						3: insert the SUBEXPRESSION
						4: copy the top part of the term down
*/
						LONG size = AT.WorkPointer - gcd;

						ss = AddRHS(AT.ebufnum,1);
						while ( (ss + size + 10) > C->Top ) ss = DoubleCbuffer(AT.ebufnum,ss);
						tt = gcd;
						NCOPY(ss,tt,size);
						C->rhs[C->numrhs+1] = ss;
						C->Pointer = ss;

						t[0] = SUBEXPRESSION;
						t[1] = SUBEXPSIZE;
						t[2] = C->numrhs;
						t[3] = 1;
						t[4] = AT.ebufnum;
						t += 5;
						tt = term + *term;
						while ( r < tt ) *t++ = *r++;
						*term = t - term;

						regval = 1;
						goto RegEnd;
					}
				}
#endif
				break;
			case MINFUNCTION:
			case MAXFUNCTION:
				if ( t[1] == FUNHEAD ) break;
				{
					WORD *ttt = t + FUNHEAD;
					WORD *tttstop = t + t[1];
					WORD tterm[4], iii;
					while ( ttt < tttstop ) {
						if ( *ttt > 0 ) {
							if ( ttt[ARGHEAD]-1 > ABS(ttt[*ttt-1]) ) goto nospec;
							ttt += *ttt;
						}
						else {
							if ( *ttt != -SNUMBER ) goto nospec;
							ttt += 2;
						}
					}
/*
					Function has only numerical arguments
					Pick up the first argument.
*/
					ttt = t + FUNHEAD;
					if ( *ttt > 0 ) {
loadnew1:
						for ( iii = 0; iii < ttt[ARGHEAD]; iii++ )
							AT.n_llnum[iii] = ttt[ARGHEAD+iii];
						ttt += *ttt;
					}
					else {
loadnew2:
						if ( ttt[1] == 0 ) {
							AT.n_llnum[0] = AT.n_llnum[1] = AT.n_llnum[2] = AT.n_llnum[3] = 0;
						}
						else {
							AT.n_llnum[0] = 4;
							if ( ttt[1] > 0 ) { AT.n_llnum[1] = ttt[1]; AT.n_llnum[3] = 3; }
							else { AT.n_llnum[1] = -ttt[1]; AT.n_llnum[3] = -3; }
							AT.n_llnum[2] = 1;
						}
						ttt += 2;
					}
/*
					Now loop over the other arguments
*/
					while ( ttt < tttstop ) {
						if ( *ttt > 0 ) {
							if ( AT.n_llnum[0] == 0 ) {
								if ( ( *t == MINFUNCTION && ttt[*ttt-1] < 0 )
								|| ( *t == MAXFUNCTION && ttt[*ttt-1] > 0 ) )
									goto loadnew1;
							}
							else {
								ttt += ARGHEAD;
								iii = CompCoef(AT.n_llnum,ttt);
								if ( ( iii > 0 && *t == MINFUNCTION )
								|| ( iii < 0 && *t == MAXFUNCTION ) ) {
									for ( iii = 0; iii < ttt[0]; iii++ )
										AT.n_llnum[iii] = ttt[iii];
								}
							}
							ttt += *ttt;
						}
						else {
							if ( AT.n_llnum[0] == 0 ) {
								if ( ( *t == MINFUNCTION && ttt[1] < 0 )
								|| ( *t == MAXFUNCTION && ttt[1] > 0 ) )
									goto loadnew2;
							}
							else if ( ttt[1] == 0 ) {
								if ( ( *t == MINFUNCTION && AT.n_llnum[*AT.n_llnum-1] > 0 )
								|| ( *t == MAXFUNCTION && AT.n_llnum[*AT.n_llnum-1] < 0 ) ) {
									AT.n_llnum[0] = 0;
								}
							}
							else {
								tterm[0] = 4; tterm[2] = 1;
								if ( ttt[1] < 0 ) { tterm[1] = -ttt[1]; tterm[3] = -3; }
								else { tterm[1] = ttt[1]; tterm[3] = 3; }
								iii = CompCoef(AT.n_llnum,tterm);
								if ( ( iii > 0 && *t == MINFUNCTION )
								|| ( iii < 0 && *t == MAXFUNCTION ) ) {
									for ( iii = 0; iii < 4; iii++ )
										AT.n_llnum[iii] = tterm[iii];
								}
							}
							ttt += 2;
						}
					}
					if ( AT.n_llnum[0] == 0 ) goto NormZero;
					ncoef = REDLENG(ncoef);
					nnum = REDLENG(AT.n_llnum[*AT.n_llnum-1]);	
					if ( MulRat(BHEAD (UWORD *)AT.n_coef,ncoef,(UWORD *)lnum,nnum,
					(UWORD *)AT.n_coef,&ncoef) ) goto FromNorm;
					ncoef = INCLENG(ncoef);
				}
				break;
			case INVERSEFACTORIAL:
				if ( t[FUNHEAD] == -SNUMBER && t[FUNHEAD+1] >= 0 ) {
					if ( Factorial(BHEAD t[FUNHEAD+1],(UWORD *)lnum,&nnum) )
						goto FromNorm;
					ncoef = REDLENG(ncoef);	
					if ( Divvy(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,nnum) ) goto FromNorm;
					ncoef = INCLENG(ncoef);
				}
				else {
nospec:				pcom[ncom++] = t;
				}
				break;
			case MAXPOWEROF:
				if ( ( t[FUNHEAD] == -SYMBOL )
					 && ( t[FUNHEAD+1] > 0 ) && ( t[1] == FUNHEAD+2 ) ) {
					*((UWORD *)lnum) = symbols[t[FUNHEAD+1]].maxpower;
					nnum = 1;
					goto MulIn;
				}
				else { pcom[ncom++] = t; }
				break;
			case MINPOWEROF:
				if ( ( t[FUNHEAD] == -SYMBOL )
					 && ( t[FUNHEAD] > 0 ) && ( t[1] == FUNHEAD+2 ) ) {
					*((UWORD *)lnum) = symbols[t[FUNHEAD+1]].minpower;
					nnum = 1;
					goto MulIn;
				}
				else { pcom[ncom++] = t; }
				break;
			case LNUMBER :
				ncoef = REDLENG(ncoef);
				if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)(t+3),t[2]) ) goto FromNorm;
				ncoef = INCLENG(ncoef);
				break;
			case SNUMBER :
				if ( t[2] < 0 ) {
					t[2] = -t[2];
					if ( t[3] & 1 ) ncoef = -ncoef;
				}
				else if ( t[2] == 0 ) {
					if ( t[3] < 0 ) goto NormInf;
					goto NormZero;
				}
				lnum[0] = t[2];
				nnum = 1;
				if ( t[3] && RaisPow(BHEAD (UWORD *)lnum,&nnum,(UWORD)(ABS(t[3]))) ) goto FromNorm;
				ncoef = REDLENG(ncoef);
				if ( t[3] < 0 ) {
					if ( Divvy(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,nnum) )
						goto FromNorm;
				}
				else if ( t[3] > 0 ) {
					if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)lnum,nnum) )
						goto FromNorm;
				}
				ncoef = INCLENG(ncoef);
				break;
			case GAMMA :
			case GAMMAI :
			case GAMMAFIVE :
			case GAMMASIX :
			case GAMMASEVEN :
				pnco[nnco++] = t;
				t += FUNHEAD+1;
				goto ScanCont;
			case LEVICIVITA :
				peps[neps++] = t;
				if ( ( t[2] & DIRTYFLAG ) == DIRTYFLAG ) {
					t[2] &= ~DIRTYFLAG;
					t[2] |= DIRTYSYMFLAG;
				}
				t += FUNHEAD;
ScanCont:		while ( t < r ) {
					if ( *t >= AM.OffsetIndex &&
						( *t >= AM.DumInd || ( *t < AM.WilInd &&
						indices[*t-AM.OffsetIndex].dimension ) ) )
						pcon[ncon++] = t;
					t++;
				}
				break;
			case EXPONENT :
				{
					WORD *rr;
					k = 1;
					rr = t + FUNHEAD;
					if ( *rr == ARGHEAD || ( *rr == -SNUMBER && rr[1] == 0 ) )
						k = 0;
					if ( *rr == -SNUMBER && rr[1] == 1 ) break;
					if ( *rr <= -FUNCTION ) k = *rr;
					NEXTARG(rr)
					if ( *rr == ARGHEAD || ( *rr == -SNUMBER && rr[1] == 0 ) ) {
						if ( k == 0 ) goto NormZZ;
						break;
					}
					if ( *rr == -SNUMBER && rr[1] > 0 && rr[1] < MAXPOWER && k < 0 ) {
						k = -k;
						if ( functions[k-FUNCTION].commute ) {
							for ( i = 0; i < rr[1]; i++ ) pnco[nnco++] = rr-1;
						}
						else {
							for ( i = 0; i < rr[1]; i++ ) pcom[ncom++] = rr-1;
						}
						break;
					}
					if ( k == 0 ) goto NormZero;
					if ( t[FUNHEAD] == -SYMBOL && *rr == -SNUMBER && t[1] == FUNHEAD+4 ) {
						if ( rr[1] < MAXPOWER ) {
							t = rr; *rr = t[FUNHEAD+1];
							goto NextSymbol;
						}
					}
/*
					if ( ( t[FUNHEAD] > 0 && t[FUNHEAD+1] != 0 )
					|| ( *rr > 0 && rr[1] != 0 ) ) {}
					else 
*/
					t[2] &= ~DIRTYSYMFLAG;

					pnco[nnco++] = t;
				}
				break;
			case DENOMINATOR :
				t[2] &= ~DIRTYSYMFLAG;
				pden[nden++] = t;
				pnco[nnco++] = t;
				break;
			case INDEX :
				t += 2;
				do {
					if ( *t == 0 ) goto NormZero;
					if ( *t == NOINDEX ) t++;
					else pind[nind++] = *t++;
				} while ( t < r );
				break;
			case SUBEXPRESSION :
				if ( t[3] == 0 ) break;
			case EXPRESSION :
				goto RegEnd;
			case ROOTFUNCTION :
/*
				Tries to take the n-th root inside the rationals
				If this is not possible, it clears all flags and
				hence tries no more.
				Notation:
				root_(power(=integer),(rational)number)
*/
				{ WORD nc;
				if ( t[2] == 0 ) goto defaultcase;
				if ( t[FUNHEAD] != -SNUMBER || t[FUNHEAD+1] < 0 ) goto defaultcase;
				if ( t[FUNHEAD+2] == -SNUMBER ) {
					if ( t[FUNHEAD+1] == 0 && t[FUNHEAD+3] == 0 ) goto NormZZ;
					if ( t[FUNHEAD+1] == 0 ) break;
					if ( t[FUNHEAD+3] < 0 ) {
						AT.WorkPointer[0] = -t[FUNHEAD+3];
						nc = -1;
					}
					else {
						AT.WorkPointer[0] = t[FUNHEAD+3];
						nc = 1;
					}
					AT.WorkPointer[1] = 1;
				}
				else if ( t[FUNHEAD+2] == t[1]-FUNHEAD-2
				&& t[FUNHEAD+2] == t[FUNHEAD+2+ARGHEAD]+ARGHEAD
				&& ABS(t[t[1]-1]) == t[FUNHEAD+2+ARGHEAD] - 1 ) {
					WORD *r1, *r2;
					if ( t[FUNHEAD+1] == 0 ) break;
					i = t[t[1]-1]; r1 = t + FUNHEAD+ARGHEAD+3;
					nc = REDLENG(i);
					i = ABS(i) - 1;
					r2 = AT.WorkPointer;
					while ( --i >= 0 ) *r2++ = *r1++;
				}
				else goto defaultcase;
				if ( TakeRatRoot((UWORD *)AT.WorkPointer,&nc,t[FUNHEAD+1]) ) {
					t[2] = 0;
					goto defaultcase;
				}
				ncoef = REDLENG(ncoef);
				if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)AT.WorkPointer,nc) )
						goto FromNorm;
				if ( nc < 0 ) nc = -nc;
				if ( Divvy(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)(AT.WorkPointer+nc),nc) )
						goto FromNorm;
				ncoef = INCLENG(ncoef);
				}
				break;
			case INTFUNCTION :
/*
				Can be resolved if the first argument is a number
				and the second argument either doesn't exist or has
				the value +1, 0, -1
				+1 : rounding up
				0  : rounding towards zero
				-1 : rounding down (same as no argument)
*/
				if ( t[1] <= FUNHEAD ) break;
				{
					WORD *rr, den, num;
					to = t + FUNHEAD;
					if ( *to > 0 ) {
						if ( *to == ARGHEAD ) goto NormZero;
						rr = to + *to;
						i = rr[-1];
						j = ABS(i);
						if ( to[ARGHEAD] != j+1 ) goto NoInteg;
						if ( rr >= r ) k = -1;
						else if ( *rr == ARGHEAD ) { k = 0; rr += ARGHEAD; }
						else if ( *rr == -SNUMBER ) { k = rr[1]; rr += 2; }
						else goto NoInteg;
						if ( rr != r ) goto NoInteg;
						if ( k > 1 || k < -1 ) goto NoInteg;
						to += ARGHEAD+1;
						j = (j-1) >> 1;
						i = ( i < 0 ) ? -j: j;
						UnPack((UWORD *)to,i,&den,&num);
/*
						Potentially the use of NoScrat2 is unsafe.
						It makes the routine not reentrant, but because it is
						used only locally and because we only call the
						low level routines DivLong and AddLong which never
						make calls involving Normalize, things are OK after all
*/
						if ( AN.NoScrat2 == 0 ) {
							AN.NoScrat2 = (UWORD *)Malloc1((AM.MaxTal+2)*sizeof(UWORD),"Normalize");
						}
						if ( DivLong((UWORD *)to,num,(UWORD *)(to+j),den
						,(UWORD *)AT.WorkPointer,&num,AN.NoScrat2,&den) ) goto FromNorm;
						if ( k < 0 && den < 0 ) {
							*AN.NoScrat2 = 1;
							den = -1;
							if ( AddLong((UWORD *)AT.WorkPointer,num
							,AN.NoScrat2,den,(UWORD *)AT.WorkPointer,&num) )
								goto FromNorm;
						}
						else if ( k > 0 && den > 0 ) {
							*AN.NoScrat2 = 1;
							den = 1;
							if ( AddLong((UWORD *)AT.WorkPointer,num,
							AN.NoScrat2,den,(UWORD *)AT.WorkPointer,&num) )
								goto FromNorm;
						}

					}
					else if ( *to == -SNUMBER ) {	/* No rounding needed */
						if ( to[1] < 0 ) { *AT.WorkPointer = -to[1]; num = -1; }
						else if ( to[1] == 0 ) goto NormZero;
						else { *AT.WorkPointer = to[1]; num = 1; }
					}
					else goto NoInteg;
					if ( num == 0 ) goto NormZero;
					ncoef = REDLENG(ncoef);
					if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)AT.WorkPointer,num) )
						goto FromNorm;
					ncoef = INCLENG(ncoef);
					break;
				}
NoInteg:;
/*
				Fall through if it cannot be resolved
*/
			default :
defaultcase:;
				if ( *t < FUNCTION ) {
					LOCK(ErrorMessageLock);
					MesPrint("Illegal code in Norm");
#ifdef DEBUGON
					{
						UBYTE OutBuf[140];
						AO.OutFill = AO.OutputLine = OutBuf;
						t = term;
						AO.OutSkip = 3;
						FiniLine();
						i = *t;
						while ( --i >= 0 ) {
							TalToLine((UWORD)(*t++));
							TokenToLine((UBYTE *)"  ");
						}
						AO.OutSkip = 0;
						FiniLine();
					}
#endif
					UNLOCK(ErrorMessageLock);
					goto NormMin;
				}
				if ( *t == REPLACEMENT && ReplaceType == -1 ) {
					ReplaceType = 0;
					from = t;
					if ( AN.RSsize < 2*t[1]+SUBEXPSIZE ) {
						if ( AN.ReplaceScrat ) M_free(AN.ReplaceScrat,"AN.ReplaceScrat");
						AN.RSsize = 2*t[1]+SUBEXPSIZE+40;
						AN.ReplaceScrat = (WORD *)Malloc1((AN.RSsize+1)*sizeof(WORD),"AN.ReplaceScrat");
					}
					t += FUNHEAD;
					ReplaceSub = AN.ReplaceScrat;
					ReplaceSub += SUBEXPSIZE;
					while ( t < r ) {
						if ( *t > 0 ) goto NoRep;
						if ( *t <= -FUNCTION ) {
							*ReplaceSub++ = FUNTOFUN;
							*ReplaceSub++ = 4;
							*ReplaceSub++ = -*t++;
							if ( *t > -FUNCTION ) goto NoRep;
							*ReplaceSub++ = -*t++;
						}
						else if ( t+4 > r ) goto NoRep;
						else {
						if ( *t == -SYMBOL ) {
							if ( t[2] == -SYMBOL && t+4 <= r )
								*ReplaceSub++ = SYMTOSYM;
							else if ( t[2] == -SNUMBER && t+4 <= r ) {
								*ReplaceSub++ = SYMTONUM;
								if ( ReplaceType == 0 ) {
									oldtoprhs = C->numrhs;
									oldcpointer = C->Pointer - C->Buffer;
								}
								ReplaceType = 1;
							}
							else if ( t[2] == ARGHEAD && t+2+ARGHEAD <= r ) {
								*ReplaceSub++ = SYMTONUM;
								*ReplaceSub++ = 4;
								*ReplaceSub++ = t[1];
								*ReplaceSub++ = 0;
								t += 2+ARGHEAD;
								continue;
							}
/*
							Next is the subexpression. We have to test that
							it isn't vector-like or index-like
*/
							else if ( t[2] > 0 ) {
								WORD *sstop, *ttstop, n;
								ss = t+2;
								sstop = ss + *ss;
								ss += ARGHEAD;
								while ( ss < sstop ) {
									tt = ss + *ss;
									ttstop = tt - ABS(tt[-1]);
									ss++;
									while ( ss < ttstop ) {
										if ( *ss == INDEX ) goto NoRep;
										ss += ss[1];
									}
									ss = tt;
								}
								subtype = SYMTOSUB;
								if ( ReplaceType == 0 ) {
									oldtoprhs = C->numrhs;
									oldcpointer = C->Pointer - C->Buffer;
								}
								ReplaceType = 1;
								ss = AddRHS(AT.ebufnum,1);
								tt = t+2;
								n = *tt - ARGHEAD;
								tt += ARGHEAD;
								while ( (ss + n + 10) > C->Top ) ss = DoubleCbuffer(AT.ebufnum,ss);
								while ( --n >= 0 ) *ss++ = *tt++;
								*ss++ = 0;
								C->rhs[C->numrhs+1] = ss;
								C->Pointer = ss;
								*ReplaceSub++ = subtype;
								*ReplaceSub++ = 4;
								*ReplaceSub++ = t[1];
								*ReplaceSub++ = C->numrhs;
								t += 2 + t[2];
								continue;
							}
							else goto NoRep;
						}
						else if ( *t == -VECTOR && t+4 <= r ) {
							if ( t[2] == -VECTOR ) *ReplaceSub++ = VECTOVEC;
							else if ( t[2] == -MINVECTOR )
								*ReplaceSub++ = VECTOMIN;
/*
							Next is a vector-like subexpression
							Search for vector nature first
*/
							else if ( t[2] > 0 ) {
								WORD *sstop, *ttstop, *w, *mm, n, count;
								WORD *v1, *v2 = 0;
								ss = t+2;
								sstop = ss + *ss;
								ss += ARGHEAD;
								while ( ss < sstop ) {
									tt = ss + *ss;
									ttstop = tt - ABS(tt[-1]);
									ss++;
									count = 0;
									while ( ss < ttstop ) {
										if ( *ss == INDEX ) {
											n = ss[1] - 2; ss += 2;
											while ( --n >= 0 ) {
												if ( *ss < MINSPEC ) count++;
												ss++;
											}
										}
										else ss += ss[1];
									}
									if ( count != 1 ) goto NoRep;
									ss = tt;
								}
								subtype = VECTOSUB;
								if ( ReplaceType == 0 ) {
									oldtoprhs = C->numrhs;
									oldcpointer = C->Pointer - C->Buffer;
								}
								ReplaceType = 1;
								mm = AddRHS(AT.ebufnum,1);
								*ReplaceSub++ = subtype;
								*ReplaceSub++ = 4;
								*ReplaceSub++ = t[1];
								*ReplaceSub++ = C->numrhs;
								w = t+2;
								n = *w - ARGHEAD;
								w += ARGHEAD;
								while ( (mm + n + 10) > C->Top )
									mm = DoubleCbuffer(AT.ebufnum,mm);
								while ( --n >= 0 ) *mm++ = *w++;
								*mm++ = 0;
								C->rhs[C->numrhs+1] = mm;
								C->Pointer = mm;
								mm = AddRHS(AT.ebufnum,1);
								w = t+2;
								n = *w - ARGHEAD;
								w += ARGHEAD;
								while ( (mm + n + 13) > C->Top )
									mm = DoubleCbuffer(AT.ebufnum,mm);
								sstop = w + n;
								while ( w < sstop ) {
									tt = w + *w; ttstop = tt - ABS(tt[-1]);
									ss = mm; mm++; w++;
									while ( w < ttstop ) {		/* Subterms */
										if ( *w != INDEX ) {
											n = w[1];
											NCOPY(mm,w,n);
										}
										else {
											v1 = mm;
											*mm++ = *w++;
											*mm++ = n = *w++;
											n -= 2;
											while ( --n >= 0 ) {
												if ( *w >= MINSPEC ) *mm++ = *w++;
												else v2 = w++;
											}
											n = WORDDIF(mm,v1);
											if ( n != v1[1] ) {
												if ( n <= 2 ) mm -= 2;
												else v1[1] = n;
												*mm++ = VECTOR;
												*mm++ = 4;
												*mm++ = *v2;
												*mm++ = FUNNYVEC;
											}
										}
									}
									while ( w < tt ) *mm++ = *w++;
									*ss = WORDDIF(mm,ss);
								}
								*mm++ = 0;
								C->rhs[C->numrhs+1] = mm;
								C->Pointer = mm;
								if ( mm > C->Top ) {
									LOCK(ErrorMessageLock);
									MesPrint("Internal error in Normalize with extra compiler buffer");
									UNLOCK(ErrorMessageLock);
									Terminate(-1);
								}
								t += 2 + t[2];
								continue;
							}
							else goto NoRep;
						}
						else if ( *t == -INDEX ) {
							if ( ( t[2] == -INDEX || t[2] == -VECTOR )
							&& t+4 <= r )
								*ReplaceSub++ = INDTOIND;
							else if ( t[1] >= AM.OffsetIndex ) {
								if ( t[2] == -SNUMBER && t+4 <= r
								&& t[3] >= 0 && t[3] < AM.OffsetIndex )
									*ReplaceSub++ = INDTOIND;
								else if ( t[2] == ARGHEAD && t+2+ARGHEAD <= r ) {
									*ReplaceSub++ = INDTOIND;
									*ReplaceSub++ = 4;
									*ReplaceSub++ = t[1];
									*ReplaceSub++ = 0;
									t += 2+ARGHEAD;
									continue;
								}
								else goto NoRep;
							}
							else goto NoRep;
						}
						else goto NoRep;
						*ReplaceSub++ = 4;
						*ReplaceSub++ = t[1];
						*ReplaceSub++ = t[3];
						t += 4;
						}
						
					}
					AN.ReplaceScrat[1] = ReplaceSub-AN.ReplaceScrat;
					break;
NoRep:
					if ( ReplaceType > 0 ) {
						C->numrhs = oldtoprhs;
						C->Pointer = C->Buffer + oldcpointer;
					}
					ReplaceType = -1;
					t = from;
				}
/*
				if ( *t == AM.termfunnum && t[1] == FUNHEAD+2
				&& t[FUNHEAD] == -DOLLAREXPRESSION ) termflag++;
*/
				if ( *t == DUMMYFUN || *t == DUMMYTEN ) {}
				else {
					if ( *t < (FUNCTION + WILDOFFSET) ) {
doflags:
						if ( ( ( t[2] & DIRTYFLAG ) != 0 ) && ( functions[*t-FUNCTION].tabl == 0 ) ) {
							t[2] &= ~DIRTYFLAG;
							t[2] |= DIRTYSYMFLAG;
						}
						if ( functions[*t-FUNCTION].commute ) { pnco[nnco++] = t; }
						else { pcom[ncom++] = t; }
					}
					else {
						if ( ( ( t[2] & DIRTYFLAG ) != 0 ) && ( functions[*t-FUNCTION-WILDOFFSET].tabl == 0 ) ) {
							t[2] &= ~DIRTYFLAG;
							t[2] |= DIRTYSYMFLAG;
						}
						if ( functions[*t-FUNCTION-WILDOFFSET].commute ) {
							pnco[nnco++] = t;
						}
						else { pcom[ncom++] = t; }
					}
				}

				/* Now hunt for contractible indices */

				if ( ( *t < (FUNCTION + WILDOFFSET)
				 && functions[*t-FUNCTION].spec >= TENSORFUNCTION ) || (
				 *t >= (FUNCTION + WILDOFFSET)
				 && functions[*t-FUNCTION-WILDOFFSET].spec >= TENSORFUNCTION ) ) {
					if ( *t >= GAMMA && *t <= GAMMASEVEN ) t++;
					t += FUNHEAD;
					while ( t < r ) {
						if ( *t >= AM.OffsetIndex && ( *t >= AM.DumInd
						|| ( *t < AM.WilInd && indices[*t-AM.OffsetIndex].dimension ) ) ) {
							pcon[ncon++] = t;
						}
						else if ( *t == FUNNYWILD ) { t++; }
						t++;
					}
				}
				else {
					t += FUNHEAD;
					while ( t < r ) {
						if ( *t > 0 ) {
/*
							Here we should worry about a recursion
							A problem is the possibility of a construct
							like  f(mu+nu)
*/
							t += *t;
						}
						else if ( *t <= -FUNCTION ) t++;
						else if ( *t == -INDEX ) {
							if ( t[1] >= AM.OffsetIndex &&
								( t[1] >= AM.DumInd || ( t[1] < AM.WilInd
								&& indices[t[1]-AM.OffsetIndex].dimension ) ) )
								pcon[ncon++] = t+1;
							t += 2;
						}
						else if ( *t == -SYMBOL ) {
							if ( t[1] >= MAXPOWER && t[1] < 2*MAXPOWER ) {
								*t = -SNUMBER;
								t[1] -= MAXPOWER;
							}
							else if ( t[1] < -MAXPOWER && t[1] > -2*MAXPOWER  ) {
								*t = -SNUMBER;
								t[1] += MAXPOWER;
							}
							else t += 2;
						}
						else t += 2;
					}
				}
				break;
		}
		t = r;
	} while ( t < m );
	if ( ANsc ) {
		AN.cTerm = ANsc;
		r = t = ANsr; m = ANsm;
		ANsc = ANsm = ANsr = 0;
		goto conscan;
	}
/*
  	#] First scan :
  	#[ Easy denominators :

	Easy denominators are denominators that can be replaced by
	negative powers of individual subterms. This may add to all
	our sublists.

*/
	if ( nden ) {
		for ( k = 0, i = 0; i < nden; i++ ) {
			t = pden[i];
			if ( ( t[2] & DIRTYFLAG ) == 0 ) continue;
			r = t + t[1]; m = t + FUNHEAD;
			if ( m >= r ) {
				for ( j = i+1; j < nden; j++ ) pden[j-1] = pden[j];
				nden--;
				for ( j = 0; j < nnco; j++ ) if ( pnco[j] == t ) break;
				for ( j++; j < nnco; j++ ) pnco[j-1] = pnco[j];
				nnco--;
				i--;
			}
			else {
				NEXTARG(m);
				if ( m >= r ) continue;
/*
				We have more than one argument. Split the function.
*/
				if ( k == 0 ) {
					k = 1; to = termout; from = term;
				}
				while ( from < t ) *to++ = *from++;
				m = t + FUNHEAD;
				while ( m < r ) {
					stop = to;
					*to++ = DENOMINATOR;
					for ( j = 1; j < FUNHEAD; j++ ) *to++ = 0;
					if ( *m < -FUNCTION ) *to++ = *m++;
					else if ( *m < 0 ) { *to++ = *m++; *to++ = *m++; }
					else {
						j = *m; while ( --j >= 0 ) *to++ = *m++;
					}
					stop[1] = WORDDIF(to,stop);
				}
				from = r;
				if ( i == nden - 1 ) {
					stop = term + *term;
					while ( from < stop ) *to++ = *from++;
					i = *termout = WORDDIF(to,termout);
					to = term; from = termout;
					while ( --i >= 0 ) *to++ = *from++;
					goto Restart;
				}
			}
		}
		for ( i = 0; i < nden; i++ ) {
			t = pden[i];
			if ( ( t[2] & DIRTYFLAG ) == 0 ) continue;
			t[2] = 0;
			if ( t[FUNHEAD] == -SYMBOL ) {
				WORD change;
				t += FUNHEAD+1;
				change = ExtraSymbol(*t,-1,nsym,ppsym,&ncoef);
				nsym += change;
				ppsym += change << 1;
				goto DropDen;
			}
			else if ( t[FUNHEAD] == -SNUMBER ) {
				t += FUNHEAD+1;
				if ( *t == 0 ) goto NormInf;
				if ( *t < 0 ) { *AT.WorkPointer = -*t; j = -1; }
				else { *AT.WorkPointer = *t; j = 1; }
				ncoef = REDLENG(ncoef);
				if ( Divvy(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)AT.WorkPointer,j) )
					goto FromNorm;
				ncoef = INCLENG(ncoef);
				goto DropDen;
			}
			else if ( t[FUNHEAD] == ARGHEAD ) goto NormInf;
			else if ( t[FUNHEAD] > 0 && t[FUNHEAD+ARGHEAD] == 
			t[FUNHEAD]-ARGHEAD ) {
				/* Only one term */
				r = t + t[1] - 1;
				t += FUNHEAD + ARGHEAD + 1;
				j = *r;
				m = r - ABS(*r) + 1;
				if ( j != 3 || ( ( *m != 1 ) || ( m[1] != 1 ) ) ) {
					ncoef = REDLENG(ncoef);
					if ( DivRat(BHEAD (UWORD *)AT.n_coef,ncoef,(UWORD *)m,REDLENG(j),(UWORD *)AT.n_coef,&ncoef) ) goto FromNorm;
					ncoef = INCLENG(ncoef);
					j = ABS(j) - 3;
					t[-FUNHEAD-ARGHEAD] -= j;
					t[-ARGHEAD-1] -= j;
					t[-1] -= j;
					m[0] = m[1] = 1;
					m[2] = 3;
				}
				while ( t < m ) {
					r = t + t[1];
					if ( *t == SYMBOL || *t == DOTPRODUCT ) {
						k = t[1];
						pden[i][1] -= k;
						pden[i][FUNHEAD] -= k;
						pden[i][FUNHEAD+ARGHEAD] -= k;
						m -= k;
						stop = m + 3;
						tt = to = t;
						from = r;
						if ( *t == SYMBOL ) {
							t += 2;
							while ( t < r ) {
								WORD change;
								change = ExtraSymbol(*t,-t[1],nsym,ppsym,&ncoef);
								nsym += change;
								ppsym += change << 1;
								t += 2;
							}
						}
						else {
							t += 2;
							while ( t < r ) {
								*ppdot++ = *t++;
								*ppdot++ = *t++;
								*ppdot++ = -*t++;
								ndot++;
							}
						}
						while ( to < stop ) *to++ = *from++;
						r = tt;
					}
					t = r;
				}
				if ( pden[i][1] == 4+FUNHEAD+ARGHEAD ) {
DropDen:
					for ( j = 0; j < nnco; j++ ) {
						if ( pden[i] == pnco[j] ) {
							--nnco;
							while ( j < nnco ) {
								pnco[j] = pnco[j+1];
								j++;
							}
							break;
						}
					}
					pden[i--] = pden[--nden];
				}
			}
		}
	}
/*
  	#] Easy denominators :
  	#[ Index Contractions :
*/
	if ( ndel ) {
		t = pdel;
		for ( i = 0; i < ndel; i += 2 ) {
			if ( t[0] == t[1] ) {
				if ( t[0] == EMPTYINDEX ) {}
				else if ( *t < AM.OffsetIndex ) {
					k = AC.FixIndices[*t];
					if ( k < 0 ) { j = -1; k = -k; }
					else if ( k > 0 ) j = 1;
					else goto NormZero;
					goto WithFix;
				}
				else if ( *t >= AM.DumInd ) {
					k = AC.lDefDim;
					if ( k ) goto docontract;
				}
				else if ( *t >= AM.WilInd ) {
					k = indices[*t-AM.OffsetIndex-WILDOFFSET].dimension;
					if ( k ) goto docontract;
				}
				else if ( ( k = indices[*t-AM.OffsetIndex].dimension ) != 0 ) {
docontract:
					if ( k > 0 ) {
						j = 1;
WithFix:				shortnum = k;
						ncoef = REDLENG(ncoef);
						if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)(&shortnum),j) )
							goto FromNorm;
						ncoef = INCLENG(ncoef);
					}
					else {
						WORD change;
						change = ExtraSymbol((WORD)(-k),(WORD)1,nsym,ppsym,&ncoef);
						nsym += change;
						ppsym += change << 1;
	   				}
					t[1] = pdel[ndel-1];
					t[0] = pdel[ndel-2];
HaveCon:
					ndel -= 2;
					i -= 2;
				}
			}
			else {
				if ( *t < AM.OffsetIndex && t[1] < AM.OffsetIndex ) goto NormZero;
				j = *t - AM.OffsetIndex;
				if ( j >= 0 && ( ( *t >= AM.DumInd && AC.lDefDim )
				|| ( *t < AM.WilInd && indices[j].dimension ) ) ) {
					for ( j = i + 2, m = pdel+j; j < ndel; j += 2, m += 2 ) {
						if ( *t == *m ) {
							*t = m[1];
							*m++ = pdel[ndel-2];
							*m = pdel[ndel-1];
							goto HaveCon;
						}
						else if ( *t == m[1] ) {
							*t = *m;
							*m++ = pdel[ndel-2];
							*m   = pdel[ndel-1];
							goto HaveCon;
						}
					}
				}
				j = t[1]-AM.OffsetIndex;
				if ( j >= 0 && ( ( t[1] >= AM.DumInd && AC.lDefDim )
				|| ( t[1] < AM.WilInd && indices[j].dimension ) ) ) {
					for ( j = i + 2, m = pdel+j; j < ndel; j += 2, m += 2 ) {
						if ( t[1] == *m ) {
							t[1] = m[1];
							*m++ = pdel[ndel-2];
							*m   = pdel[ndel-1];
							goto HaveCon;
						}
						else if ( t[1] == m[1] ) {
							t[1] = *m;
							*m++ = pdel[ndel-2];
							*m   = pdel[ndel-1];
							goto HaveCon;
						}
					}
				}
				t += 2;
			}
		}
		if ( ndel > 0 ) {
			if ( nvec ) {	
				t = pdel;
				for ( i = 0; i < ndel; i++ ) {
					if ( *t >= AM.OffsetIndex && ( ( *t >= AM.DumInd && AC.lDefDim ) ||
					( *t < AM.WilInd && indices[*t-AM.OffsetIndex].dimension ) ) ) {
						r = pvec + 1;
						for ( j = 1; j < nvec; j += 2 ) {
							if ( *r == *t ) {
								if ( i & 1 ) {
									*r = t[-1];
									*t-- = pdel[--ndel];
									i -= 2;
								}
								else {
									*r = t[1];
									t[1] = pdel[--ndel];
									i--;
								}
								*t-- = pdel[--ndel];
								break;
							}
							r += 2;
						}
					}
					t++;
				}
			}
			if ( ndel > 0 && ncon ) {
				t = pdel;
				for ( i = 0; i < ndel; i++ ) {
					if ( *t >= AM.OffsetIndex && ( ( *t >= AM.DumInd && AC.lDefDim ) ||
					( *t < AM.WilInd && indices[*t-AM.OffsetIndex].dimension ) ) ) {
						for ( j = 0; j < ncon; j++ ) {
							if ( *pcon[j] == *t ) {
								if ( i & 1 ) {
									*pcon[j] = t[-1];
									*t-- = pdel[--ndel];
									i -= 2;
								}
								else {
									*pcon[j] = t[1];
									t[1] = pdel[--ndel];
									i--;
								}
								*t-- = pdel[--ndel];
								didcontr++;
								r = pcon[j];
								for ( j = 0; j < nnco; j++ ) {
									m = pnco[j];
									if ( r > m && r < m+m[1] ) {
										m[2] |= DIRTYSYMFLAG;
										break;
									}
								}
								for ( j = 0; j < ncom; j++ ) {
									m = pcom[j];
									if ( r > m && r < m+m[1] ) {
										m[2] |= DIRTYSYMFLAG;
										break;
									}
								}
								for ( j = 0; j < neps; j++ ) {
									m = peps[j];
									if ( r > m && r < m+m[1] ) {
										m[2] |= DIRTYSYMFLAG;
										break;
									}
								}
								break;
							}
						}
					}
					t++;
				}
			}
		}
	}
	if ( nvec ) {
		t = pvec + 1;
		for ( i = 3; i < nvec; i += 2 ) {
			k = *t - AM.OffsetIndex;
			if ( k >= 0 && ( ( *t > AM.DumInd && AC.lDefDim )
			|| ( *t < AM.WilInd && indices[k].dimension ) ) ) {
				r = t + 2;
				for ( j = i; j < nvec; j += 2 ) {
					if ( *r == *t ) {	/* Another dotproduct */
						*ppdot++ = t[-1];
						*ppdot++ = r[-1];
						*ppdot++ = 1;
						ndot++;
						*r-- = pvec[--nvec];
						*r   = pvec[--nvec];
						*t-- = pvec[--nvec];
						*t-- = pvec[--nvec];
						i -= 2;
						break;
					}
					r += 2;
				}
			}
			t += 2;
		}
		if ( nvec > 0 && ncon ) {
			t = pvec + 1;
			for ( i = 1; i < nvec; i += 2 ) {
				k = *t - AM.OffsetIndex;
				if ( k >= 0 && ( ( *t >= AM.DumInd && AC.lDefDim )
				|| ( *t < AM.WilInd && indices[k].dimension ) ) ) {
					for ( j = 0; j < ncon; j++ ) {
						if ( *pcon[j] == *t ) {
							*pcon[j] = t[-1];
							*t-- = pvec[--nvec];
							*t-- = pvec[--nvec];
							r = pcon[j];
							pcon[j] = pcon[--ncon];
							i -= 2;
							for ( j = 0; j < nnco; j++ ) {
								m = pnco[j];
								if ( r > m && r < m+m[1] ) {
									m[2] |= DIRTYSYMFLAG;
									break;
								}
							}
							for ( j = 0; j < ncom; j++ ) {
								m = pcom[j];
								if ( r > m && r < m+m[1] ) {
									m[2] |= DIRTYSYMFLAG;
									break;
								}
							}
							for ( j = 0; j < neps; j++ ) {
								m = peps[j];
								if ( r > m && r < m+m[1] ) {
									m[2] |= DIRTYSYMFLAG;
									break;
								}
							}
							break;
						}
					}
				}
				t += 2;
			}
		}
	}
/*
  	#] Index Contractions :
  	#[ NonCommuting Functions :
*/
	m = fillsetexp;
	if ( nnco ) {
		for ( i = 0; i < nnco; i++ ) {
			t = pnco[i];
			if ( ( *t >= (FUNCTION+WILDOFFSET)
			&& functions[*t-FUNCTION-WILDOFFSET].spec == 0 )
			|| ( *t >= FUNCTION && *t < (FUNCTION + WILDOFFSET)
			&& functions[*t-FUNCTION].spec == 0 ) ) {
				DoRevert(t,m);
				if ( didcontr ) {
					r = t + FUNHEAD;
					t += t[1];
					while ( r < t ) {
						if ( *r == -INDEX && r[1] >= 0 && r[1] < AM.OffsetIndex ) {
							*r = -SNUMBER;
							didcontr--;
							pnco[i][2] |= DIRTYSYMFLAG;
						}
						NEXTARG(r)
					}
				}
			}
		}

		/* First should come the code for function properties. */
 
		/* First we test for symmetric properties and the DIRTYSYMFLAG */

		for ( i = 0; i < nnco; i++ ) {
			t = pnco[i];
			if ( *t > 0 && ( t[2] & DIRTYSYMFLAG ) && *t != DOLLAREXPRESSION ) {
				l = 0; /* to make the compiler happy */
				if ( ( *t >= (FUNCTION+WILDOFFSET)
				&& ( l = functions[*t-FUNCTION-WILDOFFSET].symmetric ) > 0 )
				|| ( *t >= FUNCTION && *t < (FUNCTION + WILDOFFSET)
				&& ( l = functions[*t-FUNCTION].symmetric ) > 0 ) ) {
					if ( *t >= (FUNCTION+WILDOFFSET) ) {
						*t -= WILDOFFSET;
						j = FullSymmetrize(BHEAD t,l);
						*t += WILDOFFSET;
					}
					else j = FullSymmetrize(BHEAD t,l);
					if ( (l & ~REVERSEORDER) == ANTISYMMETRIC ) {
						if ( ( j & 2 ) != 0 ) goto NormZero;
						if ( ( j & 1 ) != 0 ) ncoef = -ncoef;
					}
				}
				else t[2] &= ~DIRTYSYMFLAG;
			}
		}

		/* Non commuting functions are then tested for commutation
		   rules. If needed their order is exchanged. */

		k = nnco - 1;
		for ( i = 0; i < k; i++ ) {
			j = i;
			while ( Commute(pnco[j],pnco[j+1]) ) {
				t = pnco[j]; pnco[j] = pnco[j+1]; pnco[j+1] = t;
				l = j-1;
				while ( l >= 0 && Commute(pnco[l],pnco[l+1]) ) {
					t = pnco[l]; pnco[l] = pnco[l+1]; pnco[l+1] = t;
					l--;
				}
				if ( ++j >= k ) break;
			}
		}

		/* Finally they are written to output. gamma matrices
		   are bundled if possible */

		for ( i = 0; i < nnco; i++ ) {
			t = pnco[i];
			if ( *t >= GAMMA && *t <= GAMMASEVEN ) {
				WORD gtype;
				to = m;
				*m++ = GAMMA;
				m++;
				FILLFUN(m)
				*m++ = stype = t[FUNHEAD];		/* type of string */
				j = 0;
				nnum = 0;
				do {
					r = t + t[1];
					if ( *t == GAMMAFIVE ) {
						gtype = GAMMA5; t += FUNHEAD; goto onegammamatrix; }
					else if ( *t == GAMMASIX ) {
						gtype = GAMMA6; t += FUNHEAD; goto onegammamatrix; }
					else if ( *t == GAMMASEVEN ) {
						gtype = GAMMA7; t += FUNHEAD; goto onegammamatrix; }
					t += FUNHEAD+1;
					while ( t < r ) {
						gtype = *t;
onegammamatrix:
						if ( gtype == GAMMA5 ) {
							if ( j == GAMMA1 ) j = GAMMA5;
							else if ( j == GAMMA5 ) j = GAMMA1;
							else if ( j == GAMMA7 ) ncoef = -ncoef;
							if ( nnum & 1 ) ncoef = -ncoef;
						}
						else if ( gtype == GAMMA6 || gtype == GAMMA7 ) {
							if ( nnum & 1 ) {
								if ( gtype == GAMMA6 ) gtype = GAMMA7;
								else				   gtype = GAMMA6;
							}
							if ( j == GAMMA1 ) j = gtype;
							else if ( j == GAMMA5 ) {
								j = gtype;
								if ( j == GAMMA7 ) ncoef = -ncoef;
							}
							else if ( j != gtype ) goto NormZero;
							else {
								shortnum = 2;
								ncoef = REDLENG(ncoef);
								if ( Mully(BHEAD (UWORD *)AT.n_coef,&ncoef,(UWORD *)(&shortnum),1) ) goto FromNorm;
								ncoef = INCLENG(ncoef);
							}
						}
						else {
							*m++ = gtype; nnum++;
						}
						t++;
					}
					
				} while ( ( ++i < nnco ) && ( *(t = pnco[i]) >= GAMMA
				&& *t <= GAMMASEVEN ) && ( t[FUNHEAD] == stype ) );
				i--;
				if ( j ) {
					k = WORDDIF(m,to) - FUNHEAD-1;
					r = m;
					from = m++;
					while ( --k >= 0 ) *from-- = *--r;
					*from = j;
				}
				to[1] = WORDDIF(m,to);
			}
			else if ( *t < 0 ) {
				*m++ = -*t; *m++ = FUNHEAD; *m++ = 0;
				FILLFUN3(m)
			}
			else {
				k = t[1];
				NCOPY(m,t,k);
			}
		}

	}
/*
  	#] NonCommuting Functions :
  	#[ Commuting Functions :
*/
	if ( ncom ) {
		for ( i = 0; i < ncom; i++ ) {
			t = pcom[i];
			if ( ( *t >= (FUNCTION+WILDOFFSET)
			&& functions[*t-FUNCTION-WILDOFFSET].spec == 0 )
			|| ( *t >= FUNCTION && *t < (FUNCTION + WILDOFFSET)
			&& functions[*t-FUNCTION].spec == 0 ) ) {
				DoRevert(t,m);
				if ( didcontr ) {
					r = t + FUNHEAD;
					t += t[1];
					while ( r < t ) {
						if ( *r == -INDEX && r[1] >= 0 && r[1] < AM.OffsetIndex ) {
							*r = -SNUMBER;
							didcontr--;
							pcom[i][2] |= DIRTYSYMFLAG;
						}
						NEXTARG(r)
					}
				}
			}
		}

		/* Now we test for symmetric properties and the DIRTYSYMFLAG */

		for ( i = 0; i < ncom; i++ ) {
			t = pcom[i];
			if ( *t > 0 && ( t[2] & DIRTYSYMFLAG ) ) {
				l = 0; /* to make the compiler happy */
				if ( ( *t >= (FUNCTION+WILDOFFSET)
				&& ( l = functions[*t-FUNCTION-WILDOFFSET].symmetric ) > 0 )
				|| ( *t >= FUNCTION && *t < (FUNCTION + WILDOFFSET)
				&& ( l = functions[*t-FUNCTION].symmetric ) > 0 ) ) {
					if ( *t >= (FUNCTION+WILDOFFSET) ) {
						*t -= WILDOFFSET;
						j = FullSymmetrize(BHEAD t,l);
						*t += WILDOFFSET;
					}
					else j = FullSymmetrize(BHEAD t,l);
					if ( (l & ~REVERSEORDER) == ANTISYMMETRIC ) {
						if ( ( j & 2 ) != 0 ) goto NormZero;
						if ( ( j & 1 ) != 0 ) ncoef = -ncoef;
					}
				}
				else t[2] &= ~DIRTYSYMFLAG;
			}
		}
/*
		Sort the functions
		From a purists point of view this can be improved.
		There arel slow and fast arguments and no conversions are
		taken into account here.
*/
		for ( i = 1; i < ncom; i++ ) {
			for ( j = i; j > 0; j-- ) {
				WORD jj,kk;
				jj = j-1;
				t = pcom[jj];
				r = pcom[j];
				if ( *t < 0 ) {
					if ( *r < 0 ) { if ( *t >= *r ) goto NextI; }
					else { if ( -*t <= *r ) goto NextI; }
					goto jexch;
				}
				else if ( *r < 0 ) {
					if ( *t < -*r ) goto NextI;
					goto jexch;
				}
				else if ( *t != *r ) {
					if ( *t < *r ) goto NextI;
jexch:				t = pcom[j]; pcom[j] = pcom[jj]; pcom[jj] = t;
					continue;
				}
				if ( AC.properorderflag ) {
					if ( ( *t >= (FUNCTION+WILDOFFSET)
					&& functions[*t-FUNCTION-WILDOFFSET].spec >= TENSORFUNCTION )
					|| ( *t >= FUNCTION && *t < (FUNCTION + WILDOFFSET)
					&& functions[*t-FUNCTION].spec >= TENSORFUNCTION ) ) {}
					else {
						WORD *s1, *s2, *ss1, *ss2;
						s1 = t+FUNHEAD; s2 = r+FUNHEAD;
						ss1 = t + t[1]; ss2 = r + r[1];
						while ( s1 < ss1 && s2 < ss2 ) {
							k = CompArg(s1,s2);
							if ( k > 0 ) goto jexch;
							if ( k < 0 ) goto NextI;
							NEXTARG(s1)
							NEXTARG(s2)
						}
						if ( s1 < ss1 ) goto jexch;
						goto NextI;
					}
					k = t[1] - FUNHEAD;
					kk = r[1] - FUNHEAD;
					t += FUNHEAD;
					r += FUNHEAD;
					while ( k > 0 && kk > 0 ) {
						if ( *t < *r ) goto NextI;
						else if ( *t++ > *r++ ) goto jexch;
						k--; kk--;
					}
					if ( k > 0 ) goto jexch;
					goto NextI;
				}
				else
				{
					k = t[1] - FUNHEAD;
					kk = r[1] - FUNHEAD;
					t += FUNHEAD;
					r += FUNHEAD;
					while ( k > 0 && kk > 0 ) {
						if ( *t < *r ) goto NextI;
						else if ( *t++ > *r++ ) goto jexch;
						k--; kk--;
					}
					if ( k > 0 ) goto jexch;
					goto NextI;
				}
			}
NextI:;
		}
		for ( i = 0; i < ncom; i++ ) {
			t = pcom[i];
			if ( *t == THETA || *t == THETA2 ) {
				if ( ( k = DoTheta(BHEAD t) ) == 0 ) goto NormZero;
				else if ( k < 0 ) {
					k = t[1];
					NCOPY(m,t,k);
				}
			}
			else if ( *t == DELTA2 || *t == DELTAP ) {
				if ( ( k = DoDelta(t) ) == 0 ) goto NormZero;
				else if ( k < 0 ) {
					k = t[1];
					NCOPY(m,t,k);
				}
			}
			else if ( *t == AR.PolyFun ) {
			  if ( AR.PolyFunType == 1 ) { /* Regular PolyFun with one argument */
				if ( t[FUNHEAD+1] == 0 && AR.Eside != LHSIDE && 
				t[1] == FUNHEAD + 2 && t[FUNHEAD] == -SNUMBER ) goto NormZero;
				if ( i > 0 && pcom[i-1][0] == AR.PolyFun ) AN.PolyNormFlag = 1;
				k = t[1];
				NCOPY(m,t,k);
			  }
			  else if ( AR.PolyFunType == 2 ) { /* PolyRatFun. Two arguments */
/*
				First check for zeroes.
*/
				if ( t[FUNHEAD+1] == 0 && AR.Eside != LHSIDE && 
				t[1] > FUNHEAD + 2 && t[FUNHEAD] == -SNUMBER ) {
					u = t + FUNHEAD + 2;
					if ( *u < 0 ) {
						if ( *u <= -FUNCTION ) {}
						else if ( t[1] == FUNHEAD+4 && t[FUNHEAD+2] == -SNUMBER
							&& t[FUNHEAD+3] == 0 ) goto NormPRF;
						else if ( t[1] == FUNHEAD+4 ) goto NormZero;
					}
					else if ( t[1] == *u+FUNHEAD+2 ) goto NormZero;
				}
				if ( i > 0 && pcom[i-1][0] == AR.PolyFun ) AN.PolyNormFlag = 1;
				k = t[1];
				NCOPY(m,t,k);
			  }
			}
			else if ( *t > 0 ) {
				k = t[1];
				NCOPY(m,t,k);
			}
			else {
				*m++ = -*t; *m++ = FUNHEAD; *m++ = 0;
				FILLFUN3(m)
			}
		}
	}
/*
  	#] Commuting Functions :
  	#[ LeviCivita tensors :
*/
	if ( neps ) {
		to = m;
		for ( i = 0; i < neps; i++ ) {	/* Put the indices in order */
			t = peps[i];
			if ( ( t[2] & DIRTYSYMFLAG ) != DIRTYSYMFLAG ) continue;
			t[2] &= ~DIRTYSYMFLAG;
			if ( AR.Eside == LHSIDE || AR.Eside == LHSIDEX ) {
						/* Potential problems with FUNNYWILD */
/*
				First make sure all FUNNIES are at the end.
				Then sort separately
*/
				r = t + FUNHEAD;
				m = tt = t + t[1];
				while ( r < m ) {
					if ( *r != FUNNYWILD ) { r++; continue; }
					k = r[1]; u = r + 2;
					while ( u < tt ) {
						u[-2] = *u;
						if ( *u != FUNNYWILD ) ncoef = -ncoef;
						u++;
					}
					tt[-2] = FUNNYWILD; tt[-1] = k; m -= 2;
				}
				t += FUNHEAD;
				do {
					for ( r = t + 1; r < m; r++ ) {
						if ( *r < *t ) { k = *r; *r = *t; *t = k; ncoef = -ncoef; }
						else if ( *r == *t ) goto NormZero;
					}
					t++;
				} while ( t < m );
				do {
					for ( r = t + 2; r < tt; r += 2 ) {
						if ( r[1] < t[1] ) {
							k = r[1]; r[1] = t[1]; t[1] = k; ncoef = -ncoef; }
						else if ( r[1] == t[1] ) goto NormZero;
					}
					t += 2;
				} while ( t < tt );
			}
			else {
				m = t + t[1];
				t += FUNHEAD;
				do {
					for ( r = t + 1; r < m; r++ ) {
						if ( *r < *t ) { k = *r; *r = *t; *t = k; ncoef = -ncoef; }
						else if ( *r == *t ) goto NormZero;
					}
					t++;
				} while ( t < m );
			}
		}

		/* Sort the tensors */

		for ( i = 0; i < (neps-1); i++ ) {
			t = peps[i];
			for ( j = i+1; j < neps; j++ ) {
				r = peps[j];
				if ( t[1] > r[1] ) {
					peps[i] = m = r; peps[j] = r = t; t = m;
				}
				else if ( t[1] == r[1] ) {
					k = t[1] - FUNHEAD;
					m = t + FUNHEAD;
					r += FUNHEAD;
					do {
						if ( *r < *m ) {
							m = peps[j]; peps[j] = t; peps[i] = t = m;
							break;
						}
						else if ( *r++ > *m++ ) break;
					} while ( --k > 0 );
				}
			}
		}
		m = to;
		for ( i = 0; i < neps; i++ ) {
			t = peps[i];
			k = t[1];
			NCOPY(m,t,k);
		}
	}
/*
  	#] LeviCivita tensors :
  	#[ Delta :
*/
	if ( ndel ) {
		r = t = pdel;
		for ( i = 0; i < ndel; i += 2, r += 2 ) {
			if ( r[1] < r[0] ) { k = *r; *r = r[1]; r[1] = k; }
		}
		for ( i = 2; i < ndel; i += 2, t += 2 ) {
			r = t + 2;
			for ( j = i; j < ndel; j += 2 ) {
				if ( *r > *t ) { r += 2; }
				else if ( *r < *t ) {
					k = *r; *r++ = *t; *t++ = k;
					k = *r; *r++ = *t; *t-- = k;
				}
				else {
					if ( *++r < t[1] ) {
						k = *r; *r = t[1]; t[1] = k;
					}
					r++;
				}
			}
		}
		t = pdel;
		*m++ = DELTA;
		*m++ = ndel + 2;
		i = ndel;
		NCOPY(m,t,i);
	}
/*
  	#] Delta :
  	#[ Loose Vectors/Indices :
*/
	if ( nind ) {
		t = pind;
		for ( i = 0; i < nind; i++ ) {
			r = t + 1;
			for ( j = i+1; j < nind; j++ ) {
				if ( *r < *t ) {
					k = *r; *r = *t; *t = k;
				}
				r++;
			}
			t++;
		}
		t = pind;
		*m++ = INDEX;
		*m++ = nind + 2;
		i = nind;
		NCOPY(m,t,i);
	}
/*
  	#] Loose Vectors/Indices :
  	#[ Vectors :
*/
	if ( nvec ) {
		t = pvec;
		for ( i = 2; i < nvec; i += 2 ) {
			r = t + 2;
			for ( j = i; j < nvec; j += 2 ) {
				if ( *r == *t ) {
					if ( *++r < t[1] ) {
						k = *r; *r = t[1]; t[1] = k;
					}
					r++;
				}
				else if ( *r < *t ) {
					k = *r; *r++ = *t; *t++ = k;
					k = *r; *r++ = *t; *t-- = k;
				}
				else { r += 2; }
			}
			t += 2;
		}
		t = pvec;
		*m++ = VECTOR;
		*m++ = nvec + 2;
		i = nvec;
		NCOPY(m,t,i);
	}
/*
  	#] Vectors :
  	#[ Dotproducts :
*/
	if ( ndot ) {
		to = m;
		m = t = pdot;
		i = ndot;
		while ( --i >= 0 ) {
			if ( *t > t[1] ) { j = *t; *t = t[1]; t[1] = j; }
			t += 3;
		}
		t = m;
		ndot *= 3;
		m += ndot;
		while ( t < (m-3) ) {
			r = t + 3;
			do {
				if ( *r == *t ) {
					if ( *++r == *++t ) {
						r++;
						if ( ( *r < MAXPOWER && t[1] < MAXPOWER )
						|| ( *r > -MAXPOWER && t[1] > -MAXPOWER ) ) {
							t++;
							*t += *r;
							if ( *t > MAXPOWER || *t < -MAXPOWER ) {
								LOCK(ErrorMessageLock);
								MesPrint("Exponent of dotproduct out of range: %d",*t);
								UNLOCK(ErrorMessageLock);
								goto NormMin;
							}
							ndot -= 3;
							*r-- = *--m;
							*r-- = *--m;
							*r   = *--m;
							if ( !*t ) {
								ndot -= 3;
								*t-- = *--m;
								*t-- = *--m;
								*t   = *--m;
								t -= 3;
								break;
							}
						}
						else if ( *r < *++t ) {
							k = *r; *r++ = *t; *t = k;
						}
						else r++;
						t -= 2;
					}
					else if ( *r < *t ) {
						k = *r; *r++ = *t; *t++ = k;
						k = *r; *r++ = *t; *t = k;
						t -= 2;
					}
					else { r += 2; t--; }
				}
				else if ( *r < *t ) {
					k = *r; *r++ = *t; *t++ = k;
					k = *r; *r++ = *t; *t++ = k;
					k = *r; *r++ = *t; *t   = k;
					t -= 2;
				}
				else { r += 3; }
			} while ( r < m );
			t += 3;
		}
		m = to;
		t = pdot;
		if ( ( i = ndot ) > 0 ) {
			*m++ = DOTPRODUCT;
			*m++ = i + 2;
			NCOPY(m,t,i);
		}
	}
/*
  	#] Dotproducts :
  	#[ Symbols :
*/
	if ( nsym ) {
		nsym <<= 1;
		t = psym;
		*m++ = SYMBOL;
		r = m;
		*m++ = ( i = nsym ) + 2;
		if ( i ) { do {
			if ( !*t ) {
				if ( t[1] < (2*MAXPOWER) ) {		/* powers of i */
					if ( t[1] & 1 ) { *m++ = 0; *m++ = 1; }
					else *r -= 2;
					if ( *++t & 2 ) ncoef = -ncoef;
					t++;
				}
			}
			else if ( *t <= NumSymbols && *t > -2*MAXPOWER ) {	/* Put powers in range */
				if ( ( ( ( t[1] > symbols[*t].maxpower ) && ( symbols[*t].maxpower < MAXPOWER ) ) ||
					   ( ( t[1] < symbols[*t].minpower ) && ( symbols[*t].minpower > -MAXPOWER ) ) ) &&
					   ( t[1] < 2*MAXPOWER ) && ( t[1] > -2*MAXPOWER ) ) {
					if ( i <= 2 || t[2] != *t ) goto NormZero;
				}
				if ( AN.ncmod == 1 && ( AC.modmode & ALSOPOWERS ) != 0 ) {
					if ( AC.cmod[0] == 1 ) t[1] = 0;
					else if ( t[1] >= 0 ) t[1] = 1 + (t[1]-1)%(AC.cmod[0]-1);
					else {
						t[1] = -1 - (-t[1]-1)%(AC.cmod[0]-1);
						if ( t[1] < 0 ) t[1] += (AC.cmod[0]-1);
					}
				}
				if ( ( t[1] < (2*MAXPOWER) && t[1] >= MAXPOWER )
				|| ( t[1] > -(2*MAXPOWER) && t[1] <= -MAXPOWER ) ) {
					LOCK(ErrorMessageLock);
					MesPrint("Exponent out of range: %d",t[1]);
					UNLOCK(ErrorMessageLock);
					goto NormMin;
				}
				if ( t[1] ) {
					*m++ = *t++;
					*m++ = *t++;
				}
				else { *r -= 2; t += 2; }
			}
			else {
				*m++ = *t++; *m++ = *t++;
			}
		} while ( (i-=2) > 0 ); }
		if ( *r <= 2 ) m = r-1;
	}
/*
  	#] Symbols :
  	#[ Errors and Finish :
*/
    stop = (WORD *)(((UBYTE *)(termout)) + AM.MaxTer);
	i = ABS(ncoef);
	if ( ( m + i ) > stop ) {
		LOCK(ErrorMessageLock);
		MesPrint("Term too complex during normalization");
		UNLOCK(ErrorMessageLock);
		goto NormMin;
	}
	if ( ReplaceType >= 0 ) {
		t = AT.n_coef;
		i--;
		NCOPY(m,t,i);
		*m++ = ncoef;
		t = termout;
		*t = WORDDIF(m,t);
		if ( ReplaceType == 0 ) {
			AT.WorkPointer = termout+*termout;
			WildFill(BHEAD term,termout,AN.ReplaceScrat);
			termout = term + *term;
		}
		else {
			AT.WorkPointer = r = termout + *termout;
			WildFill(BHEAD r,termout,AN.ReplaceScrat);
			i = *r; m = term;
			NCOPY(m,r,i);
			termout = m;


			r = m = term;
			r += *term; r -= ABS(r[-1]);
			m++;
			while ( m < r ) {
				if ( *m > FUNCTION && m[1] > FUNHEAD &&
				functions[*m-FUNCTION].spec != TENSORFUNCTION )
					m[2] |= DIRTYFLAG;
				m += m[1];
			}
		}
/*
 		#[ normalize replacements :

		At this point we have the problem that we may have to treat functions
		with a dirtyflag. In the original setting DIRTYFLAG is replaced
		in the redo by DIRTYSYMFLAG but that doesn't take care of things
		like  f(y*x) -> f(x*y) etc. This is why we need to redo the arguments
		in the style that TestSub uses for dirty arguments, But this again
		involves calls to Normalize itself (and to the sorting system).
		This is the reason why Normalize has to be reentrant.

		To do things 100% right we have to call TestSub and potentially
		invoke Generator, because there could be Table elements that
		suddenly can be substituted!

		It seems best to get the term through Generator again, but that means
		we have to catch the output via the sort mechanism. It may be a bit
		wasteful, but it is definitely the most correct.
*/
#ifdef OLDNORMREPLACE
		{
			WORD oldpolynorm = AN.PolyNormFlag;
			WORD *oldcterm = AN.cTerm, *tstop, *argstop, *rnext, *tt, *wt;
			WORD *oldwork, olddefer;
			LONG newspace, oldspace, numterms;
			AT.WorkPointer = termout;
			if ( AT.WorkPointer < term + *term && term >= AT.WorkSpace
			&& term < AT.WorkTop ) AT.WorkPointer = term + *term;
/*
			To do things 100% right we have to call TestSub and potentially
			invoke Generator, because there could be Table elements that
			suddenly can be substituted!
			That last possibility we will omit!
*/
			tstop = term + *term; tstop -= ABS(tstop[-1]);
			t = term +1;
			while ( t < tstop ) {
				if ( *t >= FUNCTION && ( ( t[2] & DIRTYFLAG ) != 0 )
				&& ( functions[*t-FUNCTION].spec == 0 ) ) {
					VOID *oldcompareroutine = AR.CompareRoutine;
					r = t + FUNHEAD; argstop = t + t[1];
					while ( r < argstop ) {
						if ( *r > 0 && ( r[1] != 0 ) ) {
							m  = r + ARGHEAD; rnext = r + *r;
							oldwork = AT.WorkPointer;
							olddefer = AR.DeferFlag;
							AR.DeferFlag = 0;
							if ( *t == AR.PolyFun && AR.PolyFunType == 2 ) {
								AR.CompareRoutine = &CompareSymbols;
							}
							NewSort();
							m  = r + ARGHEAD; rnext = r + *r;
							while ( m < rnext ) {
								i = *m; tt = m; wt = oldwork;
								NCOPY(wt,tt,i);
								AT.WorkPointer = wt;
								if ( Generator(BHEAD oldwork,AR.Cnumlhs) ) {
									LowerSortLevel(); goto FromNorm;
								}
								m += *m;
							}
						    AT.WorkPointer = (WORD *)(((UBYTE *)(oldwork)) + AM.MaxTer);
							if ( AT.WorkPointer > AT.WorkTop ) goto OverWork;
							m = AT.WorkPointer;
							if ( EndSort(m,1,0) < 0 ) goto FromNorm;
							if ( *t == AR.PolyFun && AR.PolyFunType == 2 ) {
								AR.CompareRoutine = oldcompareroutine;
							}
/*
							Now we have to analyse the output
							Count terms and space
*/
							AR.DeferFlag = olddefer;
							numterms = 0; wt = m;
							while ( *wt ) { numterms++; wt += *wt; }
							newspace = wt - m;
							oldspace = *r - ARGHEAD;
/*
							Now the special cases
*/
							if ( numterms == 0 ) {
								m[0] = -SNUMBER; m[1] = 0;
								newspace = 2;
							}
							else if ( numterms == 1 ) {
								if ( *m == 4+FUNHEAD && m[3+FUNHEAD] == 3
								&& m[2+FUNHEAD] == 1 && m[1+FUNHEAD] == 1
								&& m[1] >= FUNCTION ) {
									m[0] = -m[1];
									newspace = 1;
								}
								else if ( *m == 8 && m[7] == 3
								&& m[6] == 1 && m[5] == 1
								&& m[1] == SYMBOL && m[4] == 1 ) {
									m[0] = -SYMBOL; m[1] = m[3];
									newspace = 2;
								}
								else if ( *m == 7 && m[6] == 3
								&& m[5] == 1 && m[4] == 1
								&& m[1] == INDEX ) {
									if ( m[3] >= 0 ) {
										m[0] = -INDEX; m[1] = m[3];
									}
									else {
										m[0] = -VECTOR; m[1] = m[3];
									}
									newspace = 2;
								}
								else if ( *m == 7 && m[6] == -3
								&& m[5] == 1 && m[4] == 1
								&& m[1] == INDEX && m[3] < 0 ) {
									m[0] = -MINVECTOR; m[1] = m[3];
									newspace = 2;
								}
								else if ( *m == 4
								&& m[2] == 1 && (UWORD)(m[1]) <= MAXPOSITIVE ) {
									m[0] = -SNUMBER;
									if ( m[3] < 0 ) m[1] = -m[1];
									newspace = 2;
								}
							}
/*
							Now the old argument takes oldspace spaces.
							The new argument takes newspace places.
							The new argument sits at m. There should be enough
							space in the term to accept it, but we may have to
							move the tail of the term
*/
							if ( newspace <= 2 ) {
								oldspace = *r;
								i = oldspace - newspace;
								*r = *m;
								if ( newspace > 1 ) r[1] = m[1];
								m = r + newspace;
								wt = rnext;
								tt = term + *term;
								while ( wt < tt ) *m++ = *wt++;
								*term -= i;
								t[1] -= i;
								tstop -= i;
								argstop -= i;
							}
							else if ( oldspace == newspace ) {
								i = newspace; tt = r+ARGHEAD; wt = m;
								NCOPY(tt,wt,i);
								r[1] = 0;
							}
							else if ( oldspace > newspace ) {
								i = newspace; tt = r+ARGHEAD; wt = m;
								NCOPY(tt,wt,i);
								wt = rnext; m = term + *term;
								while ( wt < m ) *tt++ = *wt++;
								i = oldspace - newspace;
								*term -= i;
								t[1] -= i;
								tstop -= i;
								argstop -= i;
								*r -= i;
								r[1] = 0;
							}
							else if ( (*term+newspace-oldspace)*sizeof(WORD) > AM.MaxTer ) {
								LOCK(ErrorMessageLock);
								MesPrint("Term too complex. Maybe increasing MaxTermSize can help");
								MesCall("Norm");
								UNLOCK(ErrorMessageLock);
								return(-1);
							}
							else {
								i = newspace - oldspace;
								tt = term + *term; wt = rnext;
								while ( tt > rnext ) { tt--; tt[i] = tt[0]; }
								*term += i;
								t[1] += i;
								tstop += i;
								argstop += i;
								*r += i;
								i = newspace; tt = r+ARGHEAD; wt = m;
								NCOPY(tt,wt,i);
								r[1] = 0;
							}
							AT.WorkPointer = oldwork;
						}
						NEXTARG(r)
					}
				}
				if ( *t >= FUNCTION && ( t[2] & DIRTYFLAG ) != 0 ) {
					t[2] &= ~DIRTYFLAG;
					if ( functions[*t-FUNCTION].symmetric ) t[2] |= DIRTYSYMFLAG;
				}
				t += t[1];
			}

			AN.PolyNormFlag = oldpolynorm;
			AN.cTerm = oldcterm;
		}
		{
			WORD *oldwork = AT.WorkPointer;
			WORD olddefer = AR.DeferFlag;
			AR.DeferFlag = 0;
			NewSort();
			if ( Generator(BHEAD term,AR.Cnumlhs) ) {
				LowerSortLevel(); goto FromNorm;
			}
		    AT.WorkPointer = oldwork;
			if ( EndSort(term,1,0) < 0 ) goto FromNorm;
			if ( *term == 0 ) goto NormZero;
			AR.DeferFlag = olddefer;
		}
#endif
/*
 		#] normalize replacements :
*/
#ifdef OLDNORMREPLACE
		AT.WorkPointer = termout;
		if ( ReplaceType == 0 ) {
			regval = 1;
			goto Restart;
		}
#endif
/*
		The next 'reset' cannot be done. We still need the expression
		in the buffer. Note though that this may cause a runaway pointer
		if we are not very careful.

		C->numrhs = oldtoprhs;
		C->Pointer = C->Buffer + oldcpointer;
*/
		return(1);
	}
	else {
		t = termout;
		k = WORDDIF(m,t);
		*t = k + i;
		m = term;
		NCOPY(m,t,k);
		i--;
		t = AT.n_coef;
		NCOPY(m,t,i);
		*m++ = ncoef;
	}
RegEnd:
	AT.WorkPointer = termout;
	if ( termout < term + *term && termout >= term ) AT.WorkPointer = term + *term;
/*
	if ( termflag ) {	We have to assign the term to $variable(s)
		TermAssign(term);
	}
*/
	return(regval);

NormInf:
	LOCK(ErrorMessageLock);
	MesPrint("Division by zero during normalization");
	UNLOCK(ErrorMessageLock);
	goto NormMin;

NormZZ:
	LOCK(ErrorMessageLock);
	MesPrint("0^0 during normalization of term");
	UNLOCK(ErrorMessageLock);
	goto NormMin;

NormPRF:
	LOCK(ErrorMessageLock);
	MesPrint("0/0 in polyratfun during normalization of term");
	UNLOCK(ErrorMessageLock);
	goto NormMin;

NormZero:
	*term = 0;
	AT.WorkPointer = termout;
	return(regval);

NormMin:
	return(-1);

FromNorm:
	LOCK(ErrorMessageLock);
	MesCall("Norm");
	UNLOCK(ErrorMessageLock);
	return(-1);

#ifdef OLDNORMREPLACE
OverWork:
	LOCK(ErrorMessageLock);
	MesWork();
	MesCall("Norm");
	UNLOCK(ErrorMessageLock);
	return(-1);
#endif

/*
  	#] Errors and Finish :
*/
}

/*
 		#] Normalize :
 		#[ ExtraSymbol :
*/

WORD ExtraSymbol(WORD sym, WORD pow, WORD nsym, WORD *ppsym, WORD *ncoef)
{
	WORD *m, i;
	i = nsym;
	m = ppsym - 2;
	while	( i > 0 ) {
		if ( sym == *m ) {
			m++;
			if	( pow > 2*MAXPOWER || pow < -2*MAXPOWER
			||	*m > 2*MAXPOWER || *m < -2*MAXPOWER ) {
				LOCK(ErrorMessageLock);
				MesPrint("Illegal wildcard power combination.");
				UNLOCK(ErrorMessageLock);
				Terminate(-1);
			}
			*m += pow;

			if ( ( sym <= NumSymbols && sym > -MAXPOWER )
			 && ( symbols[sym].complex & VARTYPEROOTOFUNITY ) == VARTYPEROOTOFUNITY ) {
				*m %= symbols[sym].maxpower;
				if ( *m < 0 ) *m += symbols[sym].maxpower;
				if ( ( symbols[sym].complex & VARTYPEMINUS ) == VARTYPEMINUS ) {
					if ( ( ( symbols[sym].maxpower & 1 ) == 0 ) &&
						( *m >= symbols[sym].maxpower/2 ) ) {
						*m -= symbols[sym].maxpower/2; *ncoef = -*ncoef;
					}
				}
			}

			if	( *m >= 2*MAXPOWER || *m <= -2*MAXPOWER ) {
				LOCK(ErrorMessageLock);
				MesPrint("Power overflow during normalization");
				UNLOCK(ErrorMessageLock);
				return(-1);
			}
			if ( !*m ) {
				m--;
				while ( i < nsym )
					{ *m = m[2]; m++; *m = m[2]; m++; i++; }
				return(-1);
			}
			return(0);
		}
		else if ( sym < *m ) {
			m -= 2;
			i--;
		}
		else break;
	}
	m = ppsym;
	while ( i < nsym )
		{ m--; m[2] = *m; m--; m[2] = *m; i++; }
	*m++ = sym;
	*m = pow;
	return(1);
}

/*
 		#] ExtraSymbol : 
 		#[ DoTheta :
*/

WORD DoTheta(PHEAD WORD *t)
{
	GETBIDENTITY
	WORD k, *r1, *r2, *tstop, type;
	WORD ia, *ta, *tb, *stopa, *stopb;
	if ( AC.BracketNormalize ) return(-1);
	type = *t;
	k = t[1];
	tstop = t + k;
	t += FUNHEAD;
	if ( k <= FUNHEAD ) return(1);
	r1 = t;
	NEXTARG(r1)
	if ( r1 == tstop ) {
/*
		One argument
*/
		if ( *t == ARGHEAD ) {
			if ( type == THETA ) return(1);
			else return(0);  /* THETA2 */
		}
		if ( *t < 0 ) {
			if ( *t == -SNUMBER ) {
				if ( t[1] < 0 ) return(0);
				else {
					if ( type == THETA2 && t[1] == 0 ) return(0);
					else return(1);
				}
			}
			return(-1);
		}
		k = t[*t-1];
		if ( *t == ABS(k)+1+ARGHEAD ) {
			if ( k > 0 ) return(1);
			else return(0);
		}
		return(-1);
	}
/*
	At least two arguments
*/
	r2 = r1;
	NEXTARG(r2)
	if ( r2 < tstop ) return(-1);	/* More than 2 arguments */
/*
	Note now that zero has to be treated specially
	We take the criteria from the symmetrize routine
*/
	if ( *t == -SNUMBER && *r1 == -SNUMBER ) {
		if ( t[1] > r1[1] ) return(0);
		else if ( t[1] < r1[1] ) {
			return(1);
		}
		else if ( type == THETA ) return(1);
		else return(0);   /* THETA2 */
	}
	else if ( t[1] == 0 && *t == -SNUMBER ) {
		if ( *r1 > 0 ) { }
		else if ( *t < *r1 ) return(1);
		else if ( *t > *r1 ) return(0);
	}
	else if ( r1[1] == 0 && *r1 == -SNUMBER ) {
		if ( *t > 0 ) { }
		else if ( *t < *r1 ) return(1);
		else if ( *t > *r1 ) return(0);
	}
	r2 = AT.WorkPointer;
	if ( *t < 0 ) {
		ta = r2;
		ToGeneral(t,ta,0);
		r2 += *r2;
	}
	else ta = t;
	if ( *r1 < 0 ) {
		tb = r2;
		ToGeneral(r1,tb,0);
	}
	else tb = r1;
	stopa = ta + *ta;
	stopb = tb + *tb;
	ta += ARGHEAD; tb += ARGHEAD;
	while ( ta < stopa ) {
		if ( tb >= stopb ) return(0);
		if ( ( ia = Compare(BHEAD ta,tb,(WORD)1) ) < 0 ) return(0);
		if ( ia > 0 ) return(1);
		ta += *ta;
		tb += *tb;
	}
	if ( type == THETA ) return(1);
	else return(0); /* THETA2 */
}

/*
 		#] DoTheta : 
 		#[ DoDelta :
*/

WORD DoDelta(WORD *t)
{
	WORD k, *r1, *r2, *tstop, isnum, isnum2, type = *t;
	if ( AC.BracketNormalize ) return(-1);
	k = t[1];
	if ( k <= FUNHEAD ) goto argzero;
	if ( k == FUNHEAD+ARGHEAD && t[FUNHEAD] == ARGHEAD ) goto argzero;
	tstop = t + k;
	t += FUNHEAD;
	r1 = t;
	NEXTARG(r1)
	if ( *t < 0 ) {
		k = 1;
		if ( *t == -SNUMBER ) { isnum = 1; k = t[1]; }
		else isnum = 0;
	}
	else {
		k = t[*t-1];
		k = ABS(k);
		if ( k == *t-ARGHEAD-1 ) isnum = 1;
		else isnum = 0;
		k = 1;
	}
	if ( r1 >= tstop ) {		/* Single argument */
		if ( !isnum ) return(-1);
		if ( k == 0 ) goto argzero;
		goto argnonzero;
	}
	r2 = r1;
	NEXTARG(r2)
	if ( r2 < tstop ) return(-1);
	if ( *r1 < 0 ) {
		if ( *r1 == -SNUMBER ) { isnum2 = 1; }
		else isnum2 = 0;
	}
	else {
		k = r1[*r1-1];
		k = ABS(k);
		if ( k == *r1-ARGHEAD-1 ) isnum2 = 1;
		else isnum2 = 0;
	}
	if ( isnum != isnum2 ) return(-1);
	tstop = r1;
	while ( t < tstop && r1 < r2 ) {
		if ( *t != *r1 ) {
			if ( !isnum ) return(-1);
			goto argnonzero;
		}
		t++; r1++;
	}
	if ( t != tstop || r1 != r2 ) {
		if ( !isnum ) return(-1);
		goto argnonzero;
	}
argzero:
	if ( type == DELTA2 ) return(1);
	else return(0);
argnonzero:
	if ( type == DELTA2 ) return(0);
	else return(1);
}

/*
 		#] DoDelta : 
 		#[ DoRevert :
*/

void DoRevert(WORD *fun, WORD *tmp)
{
	WORD *t, *r, *m, *to, *tt, *mm, i, j;
	to = fun + fun[1];
	r = fun + FUNHEAD;
	while ( r < to ) {
		if ( *r <= 0 ) {
			if ( *r == -REVERSEFUNCTION ) {
				m = r; mm = m+1;
				while ( mm < to ) *m++ = *mm++;
				to--;
				(fun[1])--;
				fun[2] |= DIRTYSYMFLAG;
			}
			else if ( *r <= -FUNCTION ) r++;
			else {
				if ( *r == -INDEX && r[1] < MINSPEC ) *r = -VECTOR;
				r += 2;
			}
		}
		else {
			if ( ( *r > ARGHEAD )
			&& ( r[ARGHEAD+1] == REVERSEFUNCTION )
			&& ( *r == (r[ARGHEAD]+ARGHEAD) )
			&& ( r[ARGHEAD] == (r[ARGHEAD+2]+4) )
			&& ( *(r+*r-3) == 1 )
			&& ( *(r+*r-2) == 1 )
			&& ( *(r+*r-1) == 3 ) ) {
				mm = r;
				r += ARGHEAD + 1;
				tt = r + r[1];
				r += FUNHEAD;
				m = tmp;
				t = r;
				j = 0;
				while ( t < tt ) {
					NEXTARG(t)
					j++;
				}
				while ( --j >= 0 ) {
					i = j;
					t = r;
					while ( --i >= 0 ) {
						NEXTARG(t)
					}
					if ( *t > 0 ) {
						i = *t;
						NCOPY(m,t,i);
					}
					else if ( *t <= -FUNCTION ) *m++ = *t++;
					else { *m++ = *t++; *m++ = *t++; }
				}
				i = WORDDIF(m,tmp);
				m = tmp;
				t = mm;
				r = t + *t;
				NCOPY(t,m,i);
				m = r;
				r = t;
				i = WORDDIF(to,m);
				NCOPY(t,m,i);
				fun[1] = WORDDIF(t,fun);
				to = t;
				fun[2] |= DIRTYSYMFLAG;
			}
			else r += *r;
		}
	}
}

/*
 		#] DoRevert : 
 	#] Normalize :
  	#[ DetCommu :

	Determines the number of terms in an expression that contain
	noncommuting objects. This can be used to see whether products of
	this expression can be evaluated with binomial coefficients.

	We don't try to be fancy. If a term contains noncommuting objects
	we are not looking whether they can commute with complete other
	terms.

	If the number gets too large we cut it off.
*/

#define MAXNUMBEROFNONCOMTERMS 2

WORD DetCommu(WORD *terms)
{
	WORD *t, *tnext, *tstop;
	WORD num = 0;
	if ( *terms == 0 ) return(0);
	if ( terms[*terms] == 0 ) return(0);
	t = terms;
	while ( *t ) {
		tnext = t + *t;
		tstop = tnext - ABS(tnext[-1]);
		t++;
		while ( t < tstop ) {
			if ( *t >= FUNCTION ) {
				if ( functions[*t-FUNCTION].commute ) {
					num++;
					if ( num >= MAXNUMBEROFNONCOMTERMS ) return(num);
					break;
				}
			}
			else if ( *t == SUBEXPRESSION ) {
				if ( cbuf[t[4]].CanCommu[t[2]] ) {
					num++;
					if ( num >= MAXNUMBEROFNONCOMTERMS ) return(num);
					break;
				}
			}
			else if ( *t == EXPRESSION ) {
				num++;
				if ( num >= MAXNUMBEROFNONCOMTERMS ) return(num);
				break;
			}
			else if ( *t == DOLLAREXPRESSION ) {
/*
				Technically this is not correct. We have to test first
				whether this is MODLOCAL (in TFORM) and if so, use the
				local version. Anyway, this should be rare to never
				occurring because dollars should be replaced.
*/
				if ( cbuf[AM.dbufnum].CanCommu[t[2]] ) {
					num++;
					if ( num >= MAXNUMBEROFNONCOMTERMS ) return(num);
					break;
				}
			}
			t += t[1];
		}
		t = tnext;
	}
	return(num);
}

/*
  	#] DetCommu : 
  	#[ EvaluateGcd :

	Try to evaluate the GCDFUNCTION gcd_.
	This function can have a number of arguments which can be numbers
	and/or polynomials. If there are objects that aren't SYMBOLS or numbers
	it cannot work currently.

	To make this work properly we have to intervene in proces.c
     proces.c:						if ( Normalize(BHEAD m) ) {
1060 proces.c:							if ( Normalize(BHEAD r) ) {
1126?proces.c:							if ( Normalize(BHEAD term) ) {
     proces.c:				if ( Normalize(BHEAD AT.WorkPointer) ) goto PasErr;
2308!proces.c:		if ( ( retnorm = Normalize(BHEAD term) ) != 0 ) {
     proces.c:					ReNumber(BHEAD term); Normalize(BHEAD term);
     proces.c:				if ( Normalize(BHEAD v) ) Terminate(-1);
     proces.c:		if ( Normalize(BHEAD w) ) { LowerSortLevel(); goto PolyCall; }
     proces.c:		if ( Normalize(BHEAD term) ) goto PolyCall;
*/

WORD *EvaluateGcd(PHEAD WORD *subterm)
{
	GETBIDENTITY
	WORD *oldworkpointer = AT.WorkPointer, *work1, *work2, *work3;
	WORD *t, *tt, *ttt, *t1, *t2, *t3, *t4, *tstop;
	WORD ct, nnum;
	UWORD gcdnum, stor;
	WORD *lnum=AT.n_llnum+1;
	WORD *num1, *num2, *num3, *den1, *den2, *den3;
	WORD sizenum1, sizenum2, sizenum3, sizeden1, sizeden2, sizeden3;
	int i, isnumeric = 0, numarg = 0 /*, sizearg */;
	long size;
/*
	Step 1: Look for -SNUMBER or -SYMBOL arguments.
	        If encountered, treat everybody with it.
*/
	tt = subterm + subterm[1]; t = subterm + FUNHEAD;

	while ( t < tt ) {
		numarg++;
		if ( *t == -SNUMBER ) {
			if ( t[1] == 0 ) {
gcdzero:;
				LOCK(ErrorMessageLock);
				MesPrint("Trying to take the GCD involving a zero term.");
				UNLOCK(ErrorMessageLock);
				return(0);
			}
			gcdnum = ABS(t[1]);
			t1 = subterm + FUNHEAD;
			while ( gcdnum > 1 && t1 < tt ) {
				if ( *t1 == -SNUMBER ) {
					stor = ABS(t1[1]);
					if ( stor == 0 ) goto gcdzero;
					if ( GcdLong(BHEAD (UWORD *)&stor,1,(UWORD *)&gcdnum,1,
									(UWORD *)lnum,&nnum) ) goto FromGCD;
					gcdnum = lnum[0];
					t1 += 2;
					continue;
				}
				else if ( *t1 == -SYMBOL ) goto gcdisone;
				else if ( *t1 < 0 ) goto gcdillegal;
/*
				Now we have to go through all the terms in the argument.
				This includes long numbers.
*/
				ttt = t1 + *t1;
				ct = *ttt; *ttt = 0;
				if ( t1[1] != 0 ) {	/* First normalize the argument */
					t1 = PolyNormPoly(BHEAD t1+ARGHEAD);
				}
				else t1 += ARGHEAD;
				while ( *t1 ) {
					t1 += *t1;
					i = ABS(t1[-1]);
					t2 = t1 - i;
					i = (i-1)/2;
					t3 = t2+i-1;
					while ( t3 > t2 && *t3 == 0 ) { t3--; i--; }
					if ( GcdLong(BHEAD (UWORD *)t2,(WORD)i,(UWORD *)&gcdnum,1,
									(UWORD *)lnum,&nnum) ) {
						*ttt = ct;
						goto FromGCD;
					}
					gcdnum = lnum[0];
					if ( gcdnum == 1 ) {
						*ttt = ct;
						goto gcdisone;
					}
				}
				*ttt = ct;
				t1 = ttt;
				AT.WorkPointer = oldworkpointer;
			}
			if ( gcdnum == 1 ) goto gcdisone;
			oldworkpointer[0] = 4;
			oldworkpointer[1] = gcdnum;
			oldworkpointer[2] = 1;
			oldworkpointer[3] = 3;
			oldworkpointer[4] = 0;
			AT.WorkPointer = oldworkpointer + 5;
			return(oldworkpointer);
		}
		else if ( *t == -SYMBOL ) {
			t1 = subterm + FUNHEAD;
			i = t[1];
			while ( t1 < tt ) {
				if ( *t1 == -SNUMBER ) goto gcdisone;
				if ( *t1 == -SYMBOL ) {
					if ( t1[1] != i ) goto gcdisone;
					t1 += 2; continue;
				}
				if ( *t1 < 0 ) goto gcdillegal;
				ttt = t1 + *t1;
				ct = *ttt; *ttt = 0;
				if ( t1[1] != 0 ) {	/* First normalize the argument */
					t2 = PolyNormPoly(BHEAD t1+ARGHEAD);
				}
				else t2 = t1 + ARGHEAD;
				while ( *t2 ) {
					t3 = t2+1;
					t2 = t2 + *t2;
					tstop = t2 - ABS(t2[-1]);
					while ( t3 < tstop ) {
						if ( *t3 != SYMBOL ) {
							*ttt = ct;
							goto gcdillegal;
						}
						t4 = t3 + 2;
						t3 += t3[1];
						while ( t4 < t3 ) {
							if ( *t4 == i && t4[1] > 0 ) goto nextterminarg;
							t4 += 2;
						}
					}
					*ttt = ct;
					goto gcdisone;
nextterminarg:;
				}
				*ttt = ct;
				t1 = ttt;
				AT.WorkPointer = oldworkpointer;
			}
			oldworkpointer[0] = 8;
			oldworkpointer[1] = SYMBOL;
			oldworkpointer[2] = 4;
			oldworkpointer[3] = t[1];
			oldworkpointer[4] = 1;
			oldworkpointer[5] = 1;
			oldworkpointer[6] = 1;
			oldworkpointer[7] = 3;
			oldworkpointer[8] = 0;
			AT.WorkPointer = oldworkpointer+9;
			return(oldworkpointer);
		}
		else if ( *t < 0 ) {
gcdillegal:;
			LOCK(ErrorMessageLock);
			MesPrint("Illegal object in gcd_ function. Object not a number or a symbol.");
			UNLOCK(ErrorMessageLock);
			goto FromGCD;
		}
		else if ( ABS(t[*t-1]) == *t-ARGHEAD-1 ) isnumeric = numarg;
		else if ( t[1] != 0 ) {
			ttt = t + *t; ct = *ttt; *ttt = 0;
			t = PolyNormPoly(BHEAD t+ARGHEAD);
			*ttt = ct;
			if ( t[*t] == 0 && ABS(t[*t-1]) == *t-ARGHEAD-1 ) isnumeric = numarg;
			AT.WorkPointer = oldworkpointer;
			t = ttt;
		}
		t += *t;
	}
/*
	At this point there are only generic arguments.
	There are however still two cases:
	1: There is an argument that is purely numerical
	   In that case we have to take the gcd of all coefficients
	2: All arguments are nontrivial polynomials.
	   Here we don't worry so much about the factor. (???)
	We know whether case 1 occurs when isnumeric > 0.
	We can look up numarg to get a good starting value.
*/
	AT.WorkPointer = oldworkpointer;
	if ( isnumeric ) {
		t = subterm + FUNHEAD;
		for ( i = 1; i < isnumeric; i++ ) {
			NEXTARG(t);
		}
		if ( t[1] != 0 ) {	/* First normalize the argument */
			ttt = t + *t; ct = *ttt; *ttt = 0;
			t = PolyNormPoly(BHEAD t+ARGHEAD);
			*ttt = ct;
		}
		t += *t;
		i = (ABS(t[-1])-1)/2;
		den1 = t - 1 - i;
		num1 = den1 - i;
		sizenum1 = sizeden1 = i;
		while ( sizenum1 > 1 && num1[sizenum1-1] == 0 ) sizenum1--;
		while ( sizeden1 > 1 && den1[sizeden1-1] == 0 ) sizeden1--;
		work1 = AT.WorkPointer+1; work2 = work1+sizenum1;
		for ( i = 0; i < sizenum1; i++ ) work1[i] = num1[i];
		for ( i = 0; i < sizeden1; i++ ) work2[i] = den1[i];
		num1 = work1; den1 = work2;
		AT.WorkPointer = work2 = work2 + sizeden1;
		t = subterm + FUNHEAD;
		while ( t < tt ) {
			ttt = t + *t; ct = *ttt; *ttt = 0;
			if ( t[1] != 0 ) {
				t = PolyNormPoly(BHEAD t+ARGHEAD);
			}
			else t += ARGHEAD;
			while ( *t ) {
				t += *t;
				i = (ABS(t[-1])-1)/2;
				den2 = t - 1 - i;
				num2 = den2 - i;
				sizenum2 = sizeden2 = i;
				while ( sizenum2 > 1 && num2[sizenum2-1] == 0 ) sizenum2--;
				while ( sizeden2 > 1 && den2[sizeden2-1] == 0 ) sizeden2--;
				num3 = AT.WorkPointer;
				if ( GcdLong(BHEAD (UWORD *)num2,sizenum2,(UWORD *)num1,sizenum1,
									(UWORD *)num3,&sizenum3) ) goto FromGCD;
				sizenum1 = sizenum3;
				for ( i = 0; i < sizenum1; i++ ) num1[i] = num3[i];
				den3 = AT.WorkPointer;
				if ( GcdLong(BHEAD (UWORD *)den2,sizeden2,(UWORD *)den1,sizeden1,
									(UWORD *)den3,&sizeden3) ) goto FromGCD;
				sizeden1 = sizeden3;
				for ( i = 0; i < sizeden1; i++ ) den1[i] = den3[i];
				if ( sizenum1 == 1 && num1[0] == 1 && sizeden1 == 1 && den1[1] == 1 )
					goto gcdisone;
			}
			*ttt = ct;
			t = ttt;
			AT.WorkPointer = work2;
		}
		AT.WorkPointer = oldworkpointer;
/*
		Now copy the GCD to the 'output'
*/
		if ( sizenum1 > sizeden1 ) {
			while ( sizenum1 > sizeden1 ) den1[sizeden1++] = 0;
		}
		else if ( sizenum1 < sizeden1 ) {
			while ( sizenum1 < sizeden1 ) num1[sizenum1++] = 0;
		}
		t = oldworkpointer;
		i = 2*sizenum1+1;
		*t++ = i+1;
		if ( num1 != t ) { NCOPY(t,num1,sizenum1); }
		else t += sizenum1;
		if ( den1 != t ) { NCOPY(t,den1,sizeden1); }
		else t += sizeden1;
		*t++ = i;
		*t++ = 0;
		AT.WorkPointer = t;
		return(oldworkpointer);
	}
/*
	Now the real stuff with only polynomials.
	Pick up the shortest term to start.
	We are a bit brutish about this.
*/
	t = subterm + FUNHEAD;
	AT.WorkPointer += AM.MaxTer/sizeof(WORD);
	work2 = AT.WorkPointer;
/*
	sizearg = subterm[1];
*/
	i = 0; work3 = 0;
	while ( t < tt ) {
		i++;
		work1 = AT.WorkPointer;
		ttt = t + *t; ct = *ttt; *ttt = 0;
		t = PolyNormPoly(BHEAD t+ARGHEAD);
		if ( *work1 < AT.WorkPointer-work1 ) {
/*
			sizearg = AT.WorkPointer-work1;
*/
			numarg = i;
			work3 = work1;
		}
		*ttt = ct; t = ttt;		
	}
	*AT.WorkPointer++ = 0;
/*
	We have properly normalized arguments and the shortest is indicated in work3
*/
	work1 = work3;
	while ( *work2 ) {
		if ( work2 != work3 ) {
			work1 = PolyGCD2(BHEAD work1,work2);
		}
		while ( *work2 ) work2 += *work2;
		work2++;
	}
	work2 = work1;
	while ( *work2 ) work2 += *work2;
	size = work2 - work1 + 1;
	t = oldworkpointer;
	NCOPY(t,work1,size);
	AT.WorkPointer = t;
	return(oldworkpointer);

gcdisone:;
	oldworkpointer[0] = 4;
	oldworkpointer[1] = 1;
	oldworkpointer[2] = 1;
	oldworkpointer[3] = 3;
	oldworkpointer[4] = 0;
	AT.WorkPointer = oldworkpointer+5;
	return(oldworkpointer);
FromGCD:
	LOCK(ErrorMessageLock);
	MesCall("EvaluateGcd");
	UNLOCK(ErrorMessageLock);
	return(0);
}

/*
  	#] EvaluateGcd : 
  	#[ DropCoefficient :
*/

void DropCoefficient(PHEAD WORD *term)
{
	GETBIDENTITY
	WORD *t = term + *term;
	WORD n, na;
	n = t[-1]; na = ABS(n);
	t -= na;
	if ( n == 3 && t[0] == 1 && t[1] == 1 ) return;
	*AN.RepPoint = 1;
	t[0] = 1; t[1] = 1; t[2] = 3;
	*term -= (na-3);
}

/*
  	#] DropCoefficient : 
*/
