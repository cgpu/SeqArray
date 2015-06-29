// ===========================================================
//
// Methods.cpp: the C/C++ codes for the SeqArray package
//
// Copyright (C) 2015    Xiuwen Zheng
//
// This file is part of SeqArray.
//
// SeqArray is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 3 as
// published by the Free Software Foundation.
//
// SeqArray is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with SeqArray.
// If not, see <http://www.gnu.org/licenses/>.

#include <Common.h>


extern "C"
{
// ======================================================================

/// Calculate the missing rate per variant
COREARRAY_DLL_EXPORT SEXP FC_NumAllele(SEXP AlleleStr)
{
	return ScalarInteger(GetNumOfAllele(CHAR(STRING_ELT(AlleleStr, 0))));
}


// ======================================================================

/// Calculate the missing rate per variant
COREARRAY_DLL_EXPORT SEXP FC_Missing_PerVariant(SEXP Geno)
{
	int *p = INTEGER(Geno);
	size_t N = XLENGTH(Geno), m = 0;
	for (size_t n=N; n > 0; n--)
	{
		if (*p++ == NA_INTEGER)
			m ++;
	}
	return ScalarReal((N > 0) ? (double(m) / N) : R_NaN);
}

/// Calculate the missing rate per sample
COREARRAY_DLL_EXPORT SEXP FC_Missing_PerSample(SEXP Geno, SEXP sum)
{
	SEXP dim = getAttrib(Geno, R_DimSymbol);
	int num_ploidy = INTEGER(dim)[0];
	int num_sample = INTEGER(dim)[1];

	int *pG = INTEGER(Geno);
	int *pS = INTEGER(sum);

	for (int i=0; i < num_sample; i++)
	{
		for (int j=0; j < num_ploidy; j++)
		{
			if (*pG++ == NA_INTEGER)
				pS[i] ++;
		}
	}

	return R_NilValue;
}


// ======================================================================

/// Get a list of allele frequencies
COREARRAY_DLL_EXPORT SEXP FC_AF_List(SEXP List)
{
	SEXP Geno = VECTOR_ELT(List, 0);
	int *p = INTEGER(Geno);
	int nAllele = GetNumOfAllele(CHAR(STRING_ELT(VECTOR_ELT(List, 1), 0)));

	SEXP rv = NEW_NUMERIC(nAllele);
	double *pV = REAL(rv);
	memset((void*)pV, 0, sizeof(double)*nAllele);
	int num = 0;

	for (size_t n=XLENGTH(Geno); n > 0; n--)
	{
		int g = *p ++;
		if (g != NA_INTEGER)
		{
			if ((0 <= g) && (g < nAllele))
			{
				num ++; pV[g] ++;
			} else
				warning("Invalid value in 'genotype/data'.");
		}
	}

	if (num > 0)
	{
		for (int i=0; i < nAllele; i++) pV[i] /= num;
	} else {
		for (int i=0; i < nAllele; i++) pV[i] = R_NaN;
	}

	return rv;
}


// ======================================================================

static int AlleleFreq_Index = 0;

/// Set the reference allele
COREARRAY_DLL_EXPORT SEXP FC_AF_SetIndex(SEXP AlelleIndex)
{
	AlleleFreq_Index = Rf_asInteger(AlelleIndex);
	return R_NilValue;
}

/// Get allele frequencies
COREARRAY_DLL_EXPORT SEXP FC_AF_Index(SEXP Geno)
{
	int *p = INTEGER(Geno);
	int n = 0, m = 0;
	for (size_t N=XLENGTH(Geno); N > 0; N--)
	{
		int g = *p ++;
		if (g != NA_INTEGER)
		{
			n ++;
			if (g == AlleleFreq_Index)
				m ++;
		}
	}
	return ScalarReal((n > 0) ? (double(m) / n) : R_NaN);
}


// ======================================================================

/// Convert a Sequencing GDS file to a SNP GDS file in `seqGDS2SNP()`
COREARRAY_DLL_EXPORT SEXP FC_AlleleStr(SEXP allele)
{
	const R_xlen_t n = XLENGTH(allele);
	for (R_xlen_t i=0; i < n; i++)
	{
		char *s = (char*)CHAR(STRING_ELT(allele, i));
		while (*s)
		{
			if (*s == ',')
				{ *s = '/'; break; }
			s ++;
		}
	}
	return allele;
}



} // extern "C"