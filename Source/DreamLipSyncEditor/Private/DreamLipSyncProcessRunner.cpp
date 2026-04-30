// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncProcessRunner.h"

#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "DreamLipSyncProcessRunner"

namespace DreamLipSyncProcessRunner
{
FText MakeElapsedMessage(const FText& BaseMessage, double ElapsedSeconds)
{
	return FText::Format(
		LOCTEXT("ExternalProcessRunningWithElapsed", "{0}\nElapsed: {1} s"),
		BaseMessage,
		FText::AsNumber(FMath::FloorToInt(ElapsedSeconds)));
}

void AppendPipeOutput(void* Pipe, FString& Output)
{
	if (Pipe)
	{
		Output += FPlatformProcess::ReadPipe(Pipe);
	}
}
}

bool FDreamLipSyncProcessRunner::RunWithProgress(
	const FString& ExecutablePath,
	const FString& Params,
	const FString& WorkingDirectory,
	const FText& Title,
	const FText& RunningMessage,
	FDreamLipSyncProcessResult& OutResult)
{
	OutResult = FDreamLipSyncProcessResult();

	FScopedSlowTask SlowTask(100.f, Title);
	SlowTask.MakeDialog(true);
	SlowTask.EnterProgressFrame(5.f, LOCTEXT("StartingExternalProcess", "Starting external lip sync process..."));

	void* StdOutReadPipe = nullptr;
	void* StdOutWritePipe = nullptr;
	void* StdErrReadPipe = nullptr;
	void* StdErrWritePipe = nullptr;

	if (!FPlatformProcess::CreatePipe(StdOutReadPipe, StdOutWritePipe) ||
		!FPlatformProcess::CreatePipe(StdErrReadPipe, StdErrWritePipe))
	{
		FPlatformProcess::ClosePipe(StdOutReadPipe, StdOutWritePipe);
		FPlatformProcess::ClosePipe(StdErrReadPipe, StdErrWritePipe);
		OutResult.ReturnCode = -1;
		OutResult.StdErr = TEXT("Could not create process output pipes.");
		return false;
	}

	const TCHAR* WorkingDirectoryPtr = WorkingDirectory.IsEmpty() ? nullptr : *WorkingDirectory;
	FProcHandle ProcessHandle = FPlatformProcess::CreateProc(
		*ExecutablePath,
		*Params,
		false,
		true,
		true,
		nullptr,
		0,
		WorkingDirectoryPtr,
		StdOutWritePipe,
		nullptr,
		StdErrWritePipe);

	if (!ProcessHandle.IsValid())
	{
		FPlatformProcess::ClosePipe(StdOutReadPipe, StdOutWritePipe);
		FPlatformProcess::ClosePipe(StdErrReadPipe, StdErrWritePipe);
		OutResult.ReturnCode = -1;
		OutResult.StdErr = FString::Printf(TEXT("Could not start process:\n%s"), *ExecutablePath);
		return false;
	}

	OutResult.bStarted = true;

	const double StartTime = FPlatformTime::Seconds();
	double LastProgressUpdateTime = StartTime;
	float RunWorkCompleted = 0.f;
	constexpr float RunWorkTotal = 90.f;

	while (FPlatformProcess::IsProcRunning(ProcessHandle))
	{
		DreamLipSyncProcessRunner::AppendPipeOutput(StdOutReadPipe, OutResult.StdOut);
		DreamLipSyncProcessRunner::AppendPipeOutput(StdErrReadPipe, OutResult.StdErr);

		const double Now = FPlatformTime::Seconds();
		if (Now - LastProgressUpdateTime >= 0.25)
		{
			const double ElapsedSeconds = Now - StartTime;
			const float TargetRunWork = FMath::Min(RunWorkTotal, static_cast<float>(RunWorkTotal * (1.0 - FMath::Exp(-ElapsedSeconds / 45.0))));
			if (TargetRunWork > RunWorkCompleted)
			{
				SlowTask.EnterProgressFrame(
					TargetRunWork - RunWorkCompleted,
					DreamLipSyncProcessRunner::MakeElapsedMessage(RunningMessage, ElapsedSeconds));
				RunWorkCompleted = TargetRunWork;
			}
			else
			{
				SlowTask.TickProgress();
			}

			LastProgressUpdateTime = Now;
		}
		else
		{
			SlowTask.TickProgress();
		}

		if (SlowTask.ShouldCancel())
		{
			OutResult.bCanceled = true;
			OutResult.ReturnCode = -1;
			FPlatformProcess::TerminateProc(ProcessHandle, true);
			break;
		}

		FPlatformProcess::Sleep(0.05f);
	}

	DreamLipSyncProcessRunner::AppendPipeOutput(StdOutReadPipe, OutResult.StdOut);
	DreamLipSyncProcessRunner::AppendPipeOutput(StdErrReadPipe, OutResult.StdErr);

	if (!OutResult.bCanceled)
	{
		int32 ProcessReturnCode = 0;
		if (FPlatformProcess::GetProcReturnCode(ProcessHandle, &ProcessReturnCode))
		{
			OutResult.ReturnCode = ProcessReturnCode;
		}
		else
		{
			OutResult.ReturnCode = -1;
		}
	}

	FPlatformProcess::CloseProc(ProcessHandle);
	FPlatformProcess::ClosePipe(StdOutReadPipe, StdOutWritePipe);
	FPlatformProcess::ClosePipe(StdErrReadPipe, StdErrWritePipe);

	if (RunWorkCompleted < RunWorkTotal)
	{
		SlowTask.EnterProgressFrame(RunWorkTotal - RunWorkCompleted, LOCTEXT("ExternalProcessFinished", "External process finished."));
	}
	SlowTask.EnterProgressFrame(5.f, LOCTEXT("ProcessingExternalOutput", "Processing generated lip sync data..."));

	return OutResult.bStarted && !OutResult.bCanceled;
}

#undef LOCTEXT_NAMESPACE
