#include "JRAds.h"

#include "Async/Async.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Containers/Ticker.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "Android/AndroidJavaEnv.h"
#endif

namespace JRAds
{
	namespace
	{
		TFunction<void(bool)> Pending;
		bool bInited = false;

		bool Simulated()
		{
			static const bool bSim = FParse::Param(FCommandLine::Get(), TEXT("jradsim"));
			return bSim;
		}

		FString UnitId()
		{
			// Default: unit iklan berhadiah UJI resmi Google. Ganti di DefaultGame.ini saat rilis.
			FString Id = TEXT("ca-app-pub-3940256099942544/5224354917");
			if (GConfig) GConfig->GetString(TEXT("JuraganRental"), TEXT("AdMobRewardedUnit"), Id, GGameIni);
			return Id;
		}
	}

	void Init()
	{
		if (bInited) return;
		bInited = true;
#if PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID M = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_JRAdsInit", "(Ljava/lang/String;)V", true);	// opsional: tanpa kode Java (mis. build lama) tidak crash
			if (M)
			{
				jstring J = Env->NewStringUTF(TCHAR_TO_UTF8(*UnitId()));
				FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, M, J);
				Env->DeleteLocalRef(J);
			}
		}
#endif
		UE_LOG(LogTemp, Display, TEXT("JR_ADS init unit=%s sim=%d"), *UnitId(), Simulated() ? 1 : 0);
	}

	bool IsReady()
	{
		if (Pending) return false;	// iklan sedang tayang
		if (Simulated()) return true;
#if PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID M = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_JRAdsReady", "()Z", true);	// opsional: tanpa kode Java (mis. build lama) tidak crash
			if (M) return FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, M);
		}
#endif
		return false;
	}

	void Show(TFunction<void(bool)> OnDone)
	{
		if (Pending) return;
		Pending = MoveTemp(OnDone);
		if (Simulated())
		{
			// Iklan pura-pura: "selesai ditonton" setelah 1 detik
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float) { OnClosed(true); return false; }), 1.f);
			return;
		}
#if PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID M = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_JRAdsShow", "()V", true);	// opsional: tanpa kode Java (mis. build lama) tidak crash
			if (M) { FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, M); return; }
		}
#endif
		OnClosed(false);
	}

	void OnClosed(bool bEarned)
	{
		TFunction<void(bool)> Done = MoveTemp(Pending);
		Pending = nullptr;
		UE_LOG(LogTemp, Display, TEXT("JR_ADS selesai hadiah=%d"), bEarned ? 1 : 0);
		if (Done) Done(bEarned);
	}
}

#if PLATFORM_ANDROID
// Dipanggil Java (GameActivity.nativeJRAdClosed) di UI thread: teruskan ke game thread.
JNI_METHOD void Java_com_epicgames_unreal_GameActivity_nativeJRAdClosed(JNIEnv* Env, jobject Thiz, jboolean bEarned)
{
	const bool b = bEarned == JNI_TRUE;
	AsyncTask(ENamedThreads::GameThread, [b]() { JRAds::OnClosed(b); });
}
#endif
