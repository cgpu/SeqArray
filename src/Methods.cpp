// ===========================================================
//
// Methods.cpp: the C/C++ codes for the SeqArray package
//
// Copyright (C) 2015-2019    Xiuwen Zheng
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

#include <Rdefines.h>
#include <R_ext/Rdynload.h>
#include "Index.h"

using namespace SeqArray;


extern "C"
{

// ======================================================================

/// Calculate the missing rate per variant
COREARRAY_DLL_EXPORT SEXP FC_Missing_PerVariant(SEXP Geno)
{
	size_t n = XLENGTH(Geno), m;
	if (TYPEOF(Geno) == RAWSXP)
		m = vec_i8_count((char*)RAW(Geno), n, NA_RAW);
	else
		m = vec_i32_count(INTEGER(Geno), n, NA_INTEGER);
	return ScalarReal((n > 0) ? (double(m) / n) : R_NaN);
}

/// Calculate the missing rate per sample
COREARRAY_DLL_EXPORT SEXP FC_Missing_PerSample(SEXP Geno, SEXP sum)
{
	int *pdim = INTEGER(GET_DIM(Geno));
	int num_ploidy=pdim[0], num_sample=pdim[1];

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
	const size_t N = XLENGTH(Geno);

	int nAllele = Rf_asInteger(VECTOR_ELT(List, 1));
	SEXP rv = NEW_NUMERIC(nAllele);
	double *pV = REAL(rv);

	size_t n1, n2, n3;
	switch (nAllele)
	{
	case 2:
		if (TYPEOF(Geno) == RAWSXP)
			vec_i8_count3((const char*)RAW(Geno), N, 0, 1, NA_RAW, &n1, &n2, &n3);
		else
			vec_i32_count3(INTEGER(Geno), N, 0, 1, NA_INTEGER, &n1, &n2, &n3);
		n3 = N - n3;
		if (n3 > 0)
		{
			pV[0] = (double)n1 / n3;
			pV[1] = (double)n2 / n3;
		} else
			pV[0] = pV[1] = R_NaN;
		break;

	case 1:
		if (TYPEOF(Geno) == RAWSXP)
			vec_i8_count2((const char*)RAW(Geno), N, 0, NA_RAW, &n1, &n2);
		else
			vec_i32_count2(INTEGER(Geno), N, 0, NA_INTEGER, &n1, &n2);
		n2 = N - n2;
		pV[0] = (n2 > 0) ? (double)n1 / n2 : R_NaN;
		break;

	default:
		int num = 0;
		memset((void*)pV, 0, sizeof(double)*nAllele);
		if (TYPEOF(Geno) == RAWSXP)
		{
			C_UInt8 *p = (C_UInt8*)RAW(Geno);
			for (size_t n=N; n > 0; n--)
			{
				C_UInt8 g = *p++;
				if (g != NA_RAW)
				{
					num ++;
					if (g < nAllele) pV[g] ++;
				}
			}
		} else {
			int *p = INTEGER(Geno);
			for (size_t n=N; n > 0; n--)
			{
				int g = *p++;
				if (g != NA_INTEGER)
				{
					num ++;
					if ((0 <= g) && (g < nAllele))
						pV[g] ++;
				}
			}
		}
		if (num > 0)
		{
			const double scale = 1.0 / num;
			for (; (nAllele--) > 0;) (*pV++) *= scale;
		} else {
			for (; (nAllele--) > 0;) (*pV++) = R_NaN;
		}
	}

	return rv;
}


// ======================================================================

static ssize_t AlleleFreq_Index = 0;
static int *AlleleFreq_RefPtr = NULL;
static SEXP AlleleFreq_Allele = R_NilValue;

/// Set the reference allele with an index
COREARRAY_DLL_EXPORT SEXP FC_AF_SetIndex(SEXP RefIndex)
{
	if (XLENGTH(RefIndex) == 1)
	{
		AlleleFreq_Index = Rf_asInteger(RefIndex);
		AlleleFreq_RefPtr = NULL;
	} else {
		AlleleFreq_Index = 0;
		AlleleFreq_RefPtr = INTEGER(RefIndex);
	}
	return R_NilValue;
}

/// Get reference allele frequency
COREARRAY_DLL_EXPORT SEXP FC_AF_Ref(SEXP Geno)
{
	const size_t N = XLENGTH(Geno);
	size_t n, m;
	if (TYPEOF(Geno) == RAWSXP)
		vec_i8_count2((const char*)RAW(Geno), N, 0, NA_RAW, &m, &n);
	else
		vec_i32_count2(INTEGER(Geno), N, 0, NA_INTEGER, &m, &n);
	n = N - n;
	return ScalarReal((n > 0) ? (double(m) / n) : R_NaN);
}

/// Get allele frequency
COREARRAY_DLL_EXPORT SEXP FC_AF_Index(SEXP List)
{
	SEXP Geno = VECTOR_ELT(List, 0);
	const int nAllele = Rf_asInteger(VECTOR_ELT(List, 1));

	const size_t N = XLENGTH(Geno);
	size_t n = 0, m = 0;
	int A = (AlleleFreq_RefPtr==NULL) ?
		AlleleFreq_Index : AlleleFreq_RefPtr[AlleleFreq_Index++];

	if (A < nAllele)
	{
		if (TYPEOF(Geno) == RAWSXP)
			vec_i8_count2((const char*)RAW(Geno), N, A, NA_RAW, &m, &n);
		else
			vec_i32_count2(INTEGER(Geno), N, A, NA_INTEGER, &m, &n);
		n = N - n;
	}

	return ScalarReal((n > 0) ? (double(m) / n) : R_NaN);
}

/// Get reference allele count
COREARRAY_DLL_EXPORT SEXP FC_AC_Ref(SEXP Geno)
{
	const size_t N = XLENGTH(Geno);
	int ans;
	if (TYPEOF(Geno) == RAWSXP)
		ans = vec_i8_count((const char*)RAW(Geno), N, 0);
	else
		ans = vec_i32_count(INTEGER(Geno), N, 0);
	return ScalarInteger(ans);
}

/// Get allele count
COREARRAY_DLL_EXPORT SEXP FC_AC_Index(SEXP List)
{
	SEXP Geno = VECTOR_ELT(List, 0);
	const int nAllele = Rf_asInteger(VECTOR_ELT(List, 1));

	const size_t N = XLENGTH(Geno);
	int A = (AlleleFreq_RefPtr==NULL) ?
		AlleleFreq_Index : AlleleFreq_RefPtr[AlleleFreq_Index++];

	int ans;
	if (A < nAllele)
	{
		if (TYPEOF(Geno) == RAWSXP)
			ans = vec_i8_count((const char*)RAW(Geno), N, A);
		else
			ans = vec_i32_count(INTEGER(Geno), N, A);
	} else
		ans = NA_INTEGER;
	
	return ScalarInteger(ans);
}


/// Set the reference allele with string
COREARRAY_DLL_EXPORT SEXP FC_AF_SetAllele(SEXP RefAllele)
{
	AlleleFreq_Allele = RefAllele;
	AlleleFreq_Index = 0;
	return R_NilValue;
}

/// Get allele frequency
COREARRAY_DLL_EXPORT SEXP FC_AF_Allele(SEXP List)
{
	SEXP Geno = VECTOR_ELT(List, 0);
	int A = GetIndexOfAllele(
		CHAR(STRING_ELT(AlleleFreq_Allele, AlleleFreq_Index++)),
		CHAR(STRING_ELT(VECTOR_ELT(List, 1), 0)));

	size_t n = 0, m = 0;
	if (A >= 0)
	{
		const size_t N = XLENGTH(Geno);
		if (TYPEOF(Geno) == RAWSXP)
		{
			if (A < 255)
				vec_i8_count2((const char*)RAW(Geno), N, A, NA_RAW, &m, &n);
			else
				n = N;
		} else
			vec_i32_count2(INTEGER(Geno), N, A, NA_INTEGER, &m, &n);
		n = N - n;
	}

	return ScalarReal((n > 0) ? (double(m) / n) : R_NaN);
}

/// Get allele count
COREARRAY_DLL_EXPORT SEXP FC_AC_Allele(SEXP List)
{
	SEXP Geno = VECTOR_ELT(List, 0);
	int A = GetIndexOfAllele(
		CHAR(STRING_ELT(AlleleFreq_Allele, AlleleFreq_Index++)),
		CHAR(STRING_ELT(VECTOR_ELT(List, 1), 0)));

	int ans;
	if (A >= 0)
	{
		const size_t N = XLENGTH(Geno);
		if (TYPEOF(Geno) == RAWSXP)
		{
			if (A < 255)
				ans = vec_i8_count((const char*)RAW(Geno), N, A);
			else
				ans = NA_INTEGER;
		} else
			ans = vec_i32_count(INTEGER(Geno), N, A);
	} else
		ans = NA_INTEGER;

	return ScalarInteger(ans);
}


// ======================================================================

/// Convert a SeqArray GDS file to a SNP GDS file in `seqGDS2SNP()`
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


// ======================================================================

/// Get a list of allele counts
COREARRAY_DLL_EXPORT SEXP FC_AlleleCount(SEXP List)
{
	SEXP Geno = VECTOR_ELT(List, 0);
	const size_t N = XLENGTH(Geno);

	int nAllele = Rf_asInteger(VECTOR_ELT(List, 1));
	SEXP rv = NEW_INTEGER(nAllele);
	int *pV = INTEGER(rv);

	size_t n1, n2;
	switch (nAllele)
	{
	case 2:
		if (TYPEOF(Geno) == RAWSXP)
			vec_i8_count2((const char*)RAW(Geno), N, 0, 1, &n1, &n2);
		else
			vec_i32_count2(INTEGER(Geno), N, 0, 1, &n1, &n2);
		pV[0] = n1; pV[1] = n2;
		break;

	case 1:
		if (TYPEOF(Geno) == RAWSXP)
			pV[0] = vec_i8_count((const char*)RAW(Geno), N, 0);
		else
			pV[0] = vec_i32_count(INTEGER(Geno), N, 0);
		break;

	default:
		memset((void*)pV, 0, sizeof(int)*nAllele);
		if (TYPEOF(Geno) == RAWSXP)
		{
			C_UInt8 *p = (C_UInt8*)RAW(Geno);
			for (size_t n=N; n > 0; n--)
			{
				C_UInt8 g = *p++;
				if (g < nAllele) pV[g] ++;
			}
		} else {
			int *p = INTEGER(Geno);
			for (size_t n=N; n > 0; n--)
			{
				int g = *p++;
				if ((0 <= g) && (g < nAllele)) pV[g] ++;
			}
		}
	}

	return rv;
}


/// Get a list of reference allele counts
COREARRAY_DLL_EXPORT SEXP FC_AlleleCount2(SEXP Geno)
{
	const size_t N = XLENGTH(Geno);
	size_t n0, nmiss;
	if (TYPEOF(Geno) == RAWSXP)
		vec_i8_count2((const char*)RAW(Geno), N, 0, 0xFF, &n0, &nmiss);
	else
		vec_i32_count2(INTEGER(Geno), N, 0, NA_INTEGER, &n0, &nmiss);
	SEXP rv = NEW_INTEGER(2);
	int *p = INTEGER(rv);
	p[0] = n0; p[1] = nmiss;
	return rv;
}



// ======================================================================
// ======================================================================

static const char *pkg_digest = "digest";
static int digest_data_type = -1;

#define PKG_LOAD(name)	\
	{ \
		DL_FUNC f =  R_FindSymbol(#name, pkg_digest, NULL); \
		if (!f) \
			error("No function '%s' in the %s package", #name, pkg_digest); \
		memcpy(&name, &f, sizeof(f)); \
	}

typedef struct _md5_context
{
	C_UInt32 total[2];
	C_UInt32 state[4];
	C_UInt8  buffer[64];
	char tmp[1024];
} md5_context;


static void (*md5_starts)(md5_context*) = NULL;
static void (*md5_update)(md5_context*, void*, C_UInt32) = NULL;
static void (*md5_finish)(md5_context*, C_UInt8[16]) = NULL;
static md5_context md5_ctx;

/// Initialize digest method
COREARRAY_DLL_EXPORT SEXP FC_DigestInit(SEXP Algo)
{
	digest_data_type = -1;
	if (!md5_starts) PKG_LOAD(md5_starts);
	if (!md5_update) PKG_LOAD(md5_update);
	if (!md5_finish) PKG_LOAD(md5_finish);
	(*md5_starts)(&md5_ctx);
	return R_NilValue;
}

/// Finalize digest method
COREARRAY_DLL_EXPORT SEXP FC_DigestDone(SEXP Algo)
{
	C_UInt8 digest[16];
	(*md5_finish)(&md5_ctx, digest);

	int Len = sizeof(digest);
	char buffer[1024 + 1];
	char *p = buffer;
	C_UInt8 *Code = digest;
	for (; Len > 0; Code++, Len--)
	{
		C_UInt8 v1 = (*Code) & 0x0F;
		C_UInt8 v2 = (*Code) >> 4;
		*p++ = (v2 < 10) ? (v2 + '0') : (v2 - 10 + 'a');
		*p++ = (v1 < 10) ? (v1 + '0') : (v1 - 10 + 'a');
	}
	*p = 0;

	return mkString(buffer);
}

/// Applied digest function
COREARRAY_DLL_EXPORT SEXP FC_DigestScan(SEXP Data)
{
	if (digest_data_type < 0)
	{
		if (TYPEOF(Data) == RAWSXP)
			digest_data_type = 0;
		else if (TYPEOF(Data) == INTSXP)
			digest_data_type = !inherits(Data, "factor") ? 1 : 2;
		else if (Rf_isLogical(Data))
			digest_data_type = 3;
		else if (Rf_isReal(Data))
			digest_data_type = 4;
		else if (Rf_isString(Data))
			digest_data_type = 5;
		else if (!Rf_isNull(Data))
			error("Not support data type.");
	}
	switch (digest_data_type)
	{
		case 0:
			(*md5_update)(&md5_ctx, RAW(Data), XLENGTH(Data));
			break;
		case 1:
			(*md5_update)(&md5_ctx, INTEGER(Data), XLENGTH(Data)*sizeof(int));
			break;
		case 2:
			{
				int *p = INTEGER(Data);
				SEXP ls = getAttrib(Data, R_LevelsSymbol);
				int nls = LENGTH(ls);
				for (size_t n=XLENGTH(Data); n > 0; n--, p++)
				{
					const char *s;
					if ((0 < *p) && (*p <= nls))
						s = CHAR(STRING_ELT(ls, *p - 1));
					else
						s = "";
					(*md5_update)(&md5_ctx, (void*)s, strlen(s)+1);
				}
			}
			break;
		case 3:
			(*md5_update)(&md5_ctx, LOGICAL(Data), XLENGTH(Data)*sizeof(int));
			break;
		case 4:
			(*md5_update)(&md5_ctx, REAL(Data), XLENGTH(Data)*sizeof(double));
			break;
		case 5:
			{
				const size_t n = XLENGTH(Data);
				for (size_t i=0; i < n; i++)
				{
					const char *s = CHAR(STRING_ELT(Data, i));
					(*md5_update)(&md5_ctx, (void*)s, strlen(s)+1);
				}
			}
	}

	return R_NilValue;
}


// ======================================================================

/// store dosage in a 2-bit packed matrix
COREARRAY_DLL_EXPORT SEXP FC_SetPackedGeno(SEXP index, SEXP dosage, SEXP rawmat)
{
	SEXP dm = Rf_getAttrib(rawmat, R_DimSymbol);
	size_t NumPacked = INTEGER(dm)[0];
	if (Rf_length(dosage) > (int)NumPacked*4)
		error("Internal error: store genotype in packed raw format!");
	int idx = Rf_asInteger(index) - 1;
	int n = Rf_length(dosage);
	Rbyte *p = RAW(rawmat) + NumPacked * idx;
	unsigned char b0, b1, b2, b3;

	if (TYPEOF(dosage) == RAWSXP)
	{
		Rbyte *s = RAW(dosage);
		for (; n >= 4; n-=4)
		{
			b0 = (0<=s[0] && s[0]<=2) ? s[0] : 3;
			b1 = (0<=s[1] && s[1]<=2) ? s[1] : 3;
			b2 = (0<=s[2] && s[2]<=2) ? s[2] : 3;
			b3 = (0<=s[3] && s[3]<=2) ? s[3] : 3;
			s += 4;
			*p++ = b0 | (b1<<2) | (b2<<4) | (b3<<6);
		}
		switch (n)
		{
			case 3:
				b0 = (0<=s[0] && s[0]<=2) ? s[0] : 3;
				b1 = (0<=s[1] && s[1]<=2) ? s[1] : 3;
				b2 = (0<=s[2] && s[2]<=2) ? s[2] : 3;
				s += 3;
				*p++ = b0 | (b1<<2) | (b2<<4) | (0x03<<6);
			case 2:
				b0 = (0<=s[0] && s[0]<=2) ? s[0] : 3;
				b1 = (0<=s[1] && s[1]<=2) ? s[1] : 3;
				s += 2;
				*p++ = b0 | (b1<<2) | (0x03<<4) | (0x03<<6);
			case 1:
				b0 = (0<=s[0] && s[0]<=2) ? s[0] : 3;
				s ++;
				*p++ = b0 | (0x03<<2) | (0x03<<4) | (0x03<<6);
		}
	} else if (TYPEOF(dosage) == INTSXP)
	{
		int *s = INTEGER(dosage);
		for (; n >= 4; n-=4)
		{
			b0 = (0<=s[0] && s[0]<=2) ? s[0] : 3;
			b1 = (0<=s[1] && s[1]<=2) ? s[1] : 3;
			b2 = (0<=s[2] && s[2]<=2) ? s[2] : 3;
			b3 = (0<=s[3] && s[3]<=2) ? s[3] : 3;
			s += 4;
			*p++ = b0 | (b1<<2) | (b2<<4) | (b3<<6);
		}
		switch (n)
		{
			case 3:
				b0 = (0<=s[0] && s[0]<=2) ? s[0] : 3;
				b1 = (0<=s[1] && s[1]<=2) ? s[1] : 3;
				b2 = (0<=s[2] && s[2]<=2) ? s[2] : 3;
				s += 3;
				*p++ = b0 | (b1<<2) | (b2<<4) | (0x03<<6);
			case 2:
				b0 = (0<=s[0] && s[0]<=2) ? s[0] : 3;
				b1 = (0<=s[1] && s[1]<=2) ? s[1] : 3;
				s += 2;
				*p++ = b0 | (b1<<2) | (0x03<<4) | (0x03<<6);
			case 1:
				b0 = (0<=s[0] && s[0]<=2) ? s[0] : 3;
				s ++;
				*p++ = b0 | (0x03<<2) | (0x03<<4) | (0x03<<6);
		}
	} else if (TYPEOF(dosage) == REALSXP)
	{
		double *ds = REAL(dosage);
		int s[4];
		for (; n >= 4; n-=4)
		{
			s[0] = (int)round(ds[0]); s[1] = (int)round(ds[1]);
			s[2] = (int)round(ds[2]); s[3] = (int)round(ds[3]);
			b0 = (0<=s[0] && s[0]<=2) ? s[0] : 3;
			b1 = (0<=s[1] && s[1]<=2) ? s[1] : 3;
			b2 = (0<=s[2] && s[2]<=2) ? s[2] : 3;
			b3 = (0<=s[3] && s[3]<=2) ? s[3] : 3;
			ds += 4;
			*p++ = b0 | (b1<<2) | (b2<<4) | (b3<<6);
		}
		switch (n)
		{
			case 3:
				s[0] = (int)round(ds[0]); s[1] = (int)round(ds[1]);
				s[2] = (int)round(ds[2]);
				b0 = (0<=s[0] && s[0]<=2) ? s[0] : 3;
				b1 = (0<=s[1] && s[1]<=2) ? s[1] : 3;
				b2 = (0<=s[2] && s[2]<=2) ? s[2] : 3;
				ds += 3;
				*p++ = b0 | (b1<<2) | (b2<<4) | (0x03<<6);
			case 2:
				s[0] = (int)round(ds[0]); s[1] = (int)round(ds[1]);
				b0 = (0<=s[0] && s[0]<=2) ? s[0] : 3;
				b1 = (0<=s[1] && s[1]<=2) ? s[1] : 3;
				ds += 2;
				*p++ = b0 | (b1<<2) | (0x03<<4) | (0x03<<6);
			case 1:
				s[0] = (int)round(ds[0]);
				b0 = (0<=s[0] && s[0]<=2) ? s[0] : 3;
				ds ++;
				*p++ = b0 | (0x03<<2) | (0x03<<4) | (0x03<<6);
		}
	} else {
		error("Internal error: invalid data type of dosage!");
	}
	return R_NilValue;
}

} // extern "C"
