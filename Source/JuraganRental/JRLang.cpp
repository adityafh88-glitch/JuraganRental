#include "JRLang.h"

#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Misc/CString.h"
#include "JRSettings.h"

namespace
{
	struct FPair { const TCHAR* Id; const TCHAR* En; };

	// Dibuat oleh Tools/make_lang_table.py dari Tools/strings_en.json
	const FPair Table[] = {
#include "JRLangTable.inc"
		{ nullptr, nullptr }
	};

	const TMap<FString, const TCHAR*>& EnglishMap()
	{
		static TMap<FString, const TCHAR*> Map;
		if (Map.Num() == 0)
		{
			for (const FPair* P = Table; P->Id; ++P) Map.Add(P->Id, P->En);
		}
		return Map;
	}
}

namespace JRLang
{
	bool IsEnglish()
	{
		static int32 Cached = -1;	// bahasa dibaca sekali per sesi
		if (Cached < 0)
		{
			const UJRSettingsSave* S = JRSettings::Get();
			if (S->Language == 0)
			{
				const FString Iso = FInternationalization::Get().GetCurrentCulture()->GetTwoLetterISOLanguageName();
				Cached = (Iso == TEXT("id") || Iso == TEXT("ms")) ? 0 : 1;
			}
			else
			{
				Cached = S->Language == 2 ? 1 : 0;
			}
			if (FParse::Param(FCommandLine::Get(), TEXT("jrenglish"))) Cached = 1;
		}
		return Cached == 1;
	}
}

const TCHAR* JRT(const TCHAR* Indonesian)
{
	if (!Indonesian || !JRLang::IsEnglish()) return Indonesian;
	if (const TCHAR* const* En = EnglishMap().Find(Indonesian)) return *En;
	return Indonesian;
}

FString JRPrintf(const TCHAR* Fmt, ...)
{
	TCHAR Buffer[2048];
	va_list Args;
	va_start(Args, Fmt);
	const int32 Len = FCString::GetVarArgs(Buffer, UE_ARRAY_COUNT(Buffer), Fmt, Args);
	va_end(Args);
	return Len >= 0 ? FString(Buffer) : FString(Fmt);
}
