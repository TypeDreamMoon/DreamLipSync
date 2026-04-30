// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncAudio2FaceImporter.h"

#include "Dom/JsonObject.h"
#include "DreamLipSyncClip.h"
#include "Misc/FileHelper.h"
#include "ScopedTransaction.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#define LOCTEXT_NAMESPACE "DreamLipSyncAudio2FaceImporter"

namespace DreamLipSyncAudio2Face
{
struct FSourceMapping
{
	FName MorphTargetName;
	float Scale = 1.f;
};

bool FieldMatches(const FString& FieldName, const TArray<FString>& CandidateNames)
{
	for (const FString& CandidateName : CandidateNames)
	{
		if (FieldName.Equals(CandidateName, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

bool ExtractStringArray(const TArray<TSharedPtr<FJsonValue>>& Values, TArray<FString>& OutStrings)
{
	TArray<FString> Strings;
	Strings.Reserve(Values.Num());

	for (const TSharedPtr<FJsonValue>& Value : Values)
	{
		if (!Value.IsValid())
		{
			continue;
		}

		if (Value->Type == EJson::String)
		{
			Strings.Add(Value->AsString());
			continue;
		}

		if (Value->Type == EJson::Object)
		{
			FString Name;
			if (Value->AsObject()->TryGetStringField(TEXT("name"), Name))
			{
				Strings.Add(Name);
			}
		}
	}

	if (Strings.IsEmpty())
	{
		return false;
	}

	OutStrings = MoveTemp(Strings);
	return true;
}

bool ExtractNumberArray(const TArray<TSharedPtr<FJsonValue>>& Values, TArray<float>& OutNumbers)
{
	TArray<float> Numbers;
	Numbers.Reserve(Values.Num());

	for (const TSharedPtr<FJsonValue>& Value : Values)
	{
		if (!Value.IsValid() || Value->Type != EJson::Number)
		{
			return false;
		}

		Numbers.Add(static_cast<float>(Value->AsNumber()));
	}

	OutNumbers = MoveTemp(Numbers);
	return true;
}

bool TryExtractRowFromObject(const TSharedPtr<FJsonObject>& Object, TArray<float>& OutRow)
{
	if (!Object.IsValid())
	{
		return false;
	}

	static const TArray<FString> RowFieldNames =
	{
		TEXT("weights"),
		TEXT("values"),
		TEXT("blendShapeWeights"),
		TEXT("blendshapeWeights"),
		TEXT("facsWeights")
	};

	for (const FString& FieldName : RowFieldNames)
	{
		const TArray<TSharedPtr<FJsonValue>>* RowValues = nullptr;
		if (Object->TryGetArrayField(FieldName, RowValues) && RowValues && ExtractNumberArray(*RowValues, OutRow))
		{
			return true;
		}
	}

	return false;
}

bool ExtractMatrix(const TArray<TSharedPtr<FJsonValue>>& Values, TArray<TArray<float>>& OutRows)
{
	TArray<TArray<float>> Rows;
	Rows.Reserve(Values.Num());

	for (const TSharedPtr<FJsonValue>& Value : Values)
	{
		if (!Value.IsValid())
		{
			continue;
		}

		TArray<float> Row;
		if (Value->Type == EJson::Array)
		{
			if (ExtractNumberArray(Value->AsArray(), Row))
			{
				Rows.Add(MoveTemp(Row));
			}
			continue;
		}

		if (Value->Type == EJson::Object && TryExtractRowFromObject(Value->AsObject(), Row))
		{
			Rows.Add(MoveTemp(Row));
		}
	}

	if (Rows.IsEmpty())
	{
		return false;
	}

	OutRows = MoveTemp(Rows);
	return true;
}

bool FindStringArrayRecursive(const TSharedPtr<FJsonObject>& Object, TArray<FString>& OutNames)
{
	if (!Object.IsValid())
	{
		return false;
	}

	static const TArray<FString> NameFieldNames =
	{
		TEXT("facsNames"),
		TEXT("blendShapeNames"),
		TEXT("blendshapeNames"),
		TEXT("bsNames"),
		TEXT("weightNames"),
		TEXT("channelNames"),
		TEXT("names")
	};

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
	{
		if (FieldMatches(Pair.Key, NameFieldNames) && Pair.Value.IsValid() && Pair.Value->Type == EJson::Array)
		{
			if (ExtractStringArray(Pair.Value->AsArray(), OutNames))
			{
				return true;
			}
		}
	}

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
	{
		if (!Pair.Value.IsValid())
		{
			continue;
		}

		if (Pair.Value->Type == EJson::Object && FindStringArrayRecursive(Pair.Value->AsObject(), OutNames))
		{
			return true;
		}

		if (Pair.Value->Type == EJson::Array)
		{
			for (const TSharedPtr<FJsonValue>& Entry : Pair.Value->AsArray())
			{
				if (Entry.IsValid() && Entry->Type == EJson::Object && FindStringArrayRecursive(Entry->AsObject(), OutNames))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool FindMatrixRecursive(const TSharedPtr<FJsonObject>& Object, TArray<TArray<float>>& OutRows)
{
	if (!Object.IsValid())
	{
		return false;
	}

	static const TArray<FString> MatrixFieldNames =
	{
		TEXT("weightMat"),
		TEXT("weightMatrix"),
		TEXT("weights"),
		TEXT("blendShapeWeights"),
		TEXT("blendshapeWeights"),
		TEXT("facsWeights"),
		TEXT("frames"),
		TEXT("data")
	};

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
	{
		if (FieldMatches(Pair.Key, MatrixFieldNames) && Pair.Value.IsValid() && Pair.Value->Type == EJson::Array)
		{
			if (ExtractMatrix(Pair.Value->AsArray(), OutRows))
			{
				return true;
			}
		}
	}

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
	{
		if (!Pair.Value.IsValid())
		{
			continue;
		}

		if (Pair.Value->Type == EJson::Object && FindMatrixRecursive(Pair.Value->AsObject(), OutRows))
		{
			return true;
		}

		if (Pair.Value->Type == EJson::Array)
		{
			for (const TSharedPtr<FJsonValue>& Entry : Pair.Value->AsArray())
			{
				if (Entry.IsValid() && Entry->Type == EJson::Object && FindMatrixRecursive(Entry->AsObject(), OutRows))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool FindNumberRecursive(const TSharedPtr<FJsonObject>& Object, const TArray<FString>& FieldNames, double& OutNumber)
{
	if (!Object.IsValid())
	{
		return false;
	}

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
	{
		if (FieldMatches(Pair.Key, FieldNames) && Pair.Value.IsValid() && Pair.Value->Type == EJson::Number)
		{
			OutNumber = Pair.Value->AsNumber();
			return true;
		}
	}

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
	{
		if (!Pair.Value.IsValid())
		{
			continue;
		}

		if (Pair.Value->Type == EJson::Object && FindNumberRecursive(Pair.Value->AsObject(), FieldNames, OutNumber))
		{
			return true;
		}

		if (Pair.Value->Type == EJson::Array)
		{
			for (const TSharedPtr<FJsonValue>& Entry : Pair.Value->AsArray())
			{
				if (Entry.IsValid() && Entry->Type == EJson::Object && FindNumberRecursive(Entry->AsObject(), FieldNames, OutNumber))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void TransposeMatrix(TArray<TArray<float>>& Rows)
{
	if (Rows.IsEmpty() || Rows[0].IsEmpty())
	{
		return;
	}

	const int32 NewRowCount = Rows[0].Num();
	const int32 NewColumnCount = Rows.Num();
	TArray<TArray<float>> Transposed;
	Transposed.SetNum(NewRowCount);

	for (int32 RowIndex = 0; RowIndex < NewRowCount; ++RowIndex)
	{
		Transposed[RowIndex].SetNumZeroed(NewColumnCount);
		for (int32 ColumnIndex = 0; ColumnIndex < NewColumnCount; ++ColumnIndex)
		{
			if (Rows.IsValidIndex(ColumnIndex) && Rows[ColumnIndex].IsValidIndex(RowIndex))
			{
				Transposed[RowIndex][ColumnIndex] = Rows[ColumnIndex][RowIndex];
			}
		}
	}

	Rows = MoveTemp(Transposed);
}

float ResolveFrameRate(const TSharedPtr<FJsonObject>& RootObject, int32 FrameCount)
{
	static const TArray<FString> FpsFieldNames =
	{
		TEXT("fps"),
		TEXT("frameRate"),
		TEXT("framerate"),
		TEXT("framesPerSecond")
	};

	double Fps = 0.0;
	if (FindNumberRecursive(RootObject, FpsFieldNames, Fps) && Fps > 0.0)
	{
		return static_cast<float>(Fps);
	}

	static const TArray<FString> DurationFieldNames =
	{
		TEXT("duration"),
		TEXT("audioDuration")
	};

	double Duration = 0.0;
	if (FrameCount > 0 && FindNumberRecursive(RootObject, DurationFieldNames, Duration) && Duration > 0.0)
	{
		return static_cast<float>(FrameCount / Duration);
	}

	return 60.f;
}

void AddClampedWeight(FName MorphTargetName, float Weight, TMap<FName, float>& OutWeights)
{
	if (MorphTargetName.IsNone())
	{
		return;
	}

	float& ExistingWeight = OutWeights.FindOrAdd(MorphTargetName);
	ExistingWeight = FMath::Clamp(ExistingWeight + Weight, 0.f, 1.f);
}

TArray<FDreamLipSyncMorphWeight> ConvertWeightMap(const TMap<FName, float>& WeightMap)
{
	TArray<FDreamLipSyncMorphWeight> Weights;
	Weights.Reserve(WeightMap.Num());

	for (const TPair<FName, float>& Pair : WeightMap)
	{
		FDreamLipSyncMorphWeight& Weight = Weights.AddDefaulted_GetRef();
		Weight.MorphTargetName = Pair.Key;
		Weight.Weight = Pair.Value;
	}

	return Weights;
}

void BuildMappingLookup(const UDreamLipSyncClip* Clip, TMap<FName, TArray<FSourceMapping>>& OutMappingLookup, TSet<FName>& OutKnownTargetMorphs)
{
	if (!Clip)
	{
		return;
	}

	for (const FDreamLipSyncVisemeMapping& Mapping : Clip->VisemeMappings)
	{
		if (!Mapping.MorphTargetName.IsNone())
		{
			OutKnownTargetMorphs.Add(Mapping.MorphTargetName);
		}
	}

	for (const FDreamLipSyncSourceMorphMapping& Mapping : Clip->SourceMorphMappings)
	{
		if (Mapping.SourceName.IsNone() || Mapping.MorphTargetName.IsNone())
		{
			continue;
		}

		FSourceMapping& Target = OutMappingLookup.FindOrAdd(Mapping.SourceName).AddDefaulted_GetRef();
		Target.MorphTargetName = Mapping.MorphTargetName;
		Target.Scale = Mapping.Scale;
		OutKnownTargetMorphs.Add(Mapping.MorphTargetName);
	}
}

TArray<FDreamLipSyncMorphFrame> BuildMorphFrames(
	const UDreamLipSyncClip* Clip,
	const TArray<FString>& SourceNames,
	const TArray<TArray<float>>& Rows,
	float FrameRate)
{
	TMap<FName, TArray<FSourceMapping>> MappingLookup;
	TSet<FName> KnownTargetMorphs;
	BuildMappingLookup(Clip, MappingLookup, KnownTargetMorphs);

	TArray<FDreamLipSyncMorphFrame> Frames;
	Frames.Reserve(Rows.Num());

	for (int32 FrameIndex = 0; FrameIndex < Rows.Num(); ++FrameIndex)
	{
		const TArray<float>& SourceWeights = Rows[FrameIndex];
		TMap<FName, float> AccumulatedWeights;

		const int32 ChannelCount = FMath::Min(SourceNames.Num(), SourceWeights.Num());
		for (int32 ChannelIndex = 0; ChannelIndex < ChannelCount; ++ChannelIndex)
		{
			const FName SourceName(*SourceNames[ChannelIndex]);
			const float SourceWeight = FMath::Clamp(SourceWeights[ChannelIndex], 0.f, 1.f);

			if (const TArray<FSourceMapping>* Mappings = MappingLookup.Find(SourceName))
			{
				for (const FSourceMapping& Mapping : *Mappings)
				{
					AddClampedWeight(Mapping.MorphTargetName, SourceWeight * Mapping.Scale, AccumulatedWeights);
				}
				continue;
			}

			if (KnownTargetMorphs.Contains(SourceName))
			{
				AddClampedWeight(SourceName, SourceWeight, AccumulatedWeights);
			}
		}

		FDreamLipSyncMorphFrame& Frame = Frames.AddDefaulted_GetRef();
		Frame.Time = FrameRate > KINDA_SMALL_NUMBER ? FrameIndex / FrameRate : 0.f;
		Frame.Weights = ConvertWeightMap(AccumulatedWeights);
	}

	return Frames;
}
}

bool FDreamLipSyncAudio2FaceImporter::ImportAudio2FaceJsonFile(UDreamLipSyncClip* Clip, const FString& JsonFilePath, FString& OutMessage)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *JsonFilePath))
	{
		OutMessage = FString::Printf(TEXT("Could not read Audio2Face JSON file:\n%s"), *JsonFilePath);
		return false;
	}

	return ImportAudio2FaceJsonString(Clip, JsonString, OutMessage);
}

bool FDreamLipSyncAudio2FaceImporter::ImportAudio2FaceJsonString(UDreamLipSyncClip* Clip, const FString& JsonString, FString& OutMessage)
{
	if (!Clip)
	{
		OutMessage = TEXT("Invalid DreamLipSyncClip.");
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		OutMessage = TEXT("Failed to parse Audio2Face JSON.");
		return false;
	}

	TArray<FString> SourceNames;
	if (!DreamLipSyncAudio2Face::FindStringArrayRecursive(RootObject, SourceNames) || SourceNames.IsEmpty())
	{
		OutMessage = TEXT("Audio2Face JSON does not contain a recognized blendshape name array.");
		return false;
	}

	TArray<TArray<float>> Rows;
	if (!DreamLipSyncAudio2Face::FindMatrixRecursive(RootObject, Rows) || Rows.IsEmpty())
	{
		OutMessage = TEXT("Audio2Face JSON does not contain a recognized blendshape weight matrix.");
		return false;
	}

	if (Rows.Num() == SourceNames.Num() && !Rows[0].IsEmpty() && Rows[0].Num() != SourceNames.Num())
	{
		DreamLipSyncAudio2Face::TransposeMatrix(Rows);
	}

	const float FrameRate = DreamLipSyncAudio2Face::ResolveFrameRate(RootObject, Rows.Num());
	TArray<FDreamLipSyncMorphFrame> Frames = DreamLipSyncAudio2Face::BuildMorphFrames(Clip, SourceNames, Rows, FrameRate);
	if (Frames.IsEmpty())
	{
		OutMessage = TEXT("Audio2Face JSON parsed, but no MorphFrames could be generated.");
		return false;
	}

	int32 WeightedFrameCount = 0;
	for (const FDreamLipSyncMorphFrame& Frame : Frames)
	{
		if (!Frame.Weights.IsEmpty())
		{
			++WeightedFrameCount;
		}
	}

	if (WeightedFrameCount == 0)
	{
		OutMessage = TEXT("Audio2Face JSON parsed, but none of its source names matched SourceMorphMappings or known target MorphTargets.");
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("ImportDreamLipSyncFromAudio2Face", "Import Dream Lip Sync From Audio2Face"));
	Clip->Modify();
	Clip->DataMode = EDreamLipSyncDataMode::MorphFrames;
	Clip->MorphFrames = MoveTemp(Frames);
	Clip->VisemeKeys.Reset();
	Clip->Duration = FrameRate > KINDA_SMALL_NUMBER ? Clip->MorphFrames.Num() / FrameRate : 0.f;
	Clip->bHoldLastKey = false;
	Clip->MarkPackageDirty();

	OutMessage = FString::Printf(TEXT("Imported %d Audio2Face frames at %.2f fps. %d frames contain mapped weights."), Clip->MorphFrames.Num(), FrameRate, WeightedFrameCount);
	return true;
}

#undef LOCTEXT_NAMESPACE
