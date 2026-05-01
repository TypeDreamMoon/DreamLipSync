// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncSoundWaveActions.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserMenuContexts.h"
#include "DreamLipSyncAceGenerator.h"
#include "DreamLipSyncClip.h"
#include "DreamLipSyncMfaGenerator.h"
#include "DreamLipSyncRhubarbGenerator.h"
#include "Editor.h"
#include "EditorFramework/AssetImportData.h"
#include "Framework/Application/SlateApplication.h"
#include "IAssetTools.h"
#include "Misc/MessageDialog.h"
#include "Sound/SoundWave.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DreamLipSyncSoundWaveActions"

namespace
{
enum class ECreateGenerationMode : uint8
{
	None,
	Rhubarb,
	MFA,
	Ace
};

struct FCreateClipOptions
{
	FString AssetName;
	FString PackagePath;
	FString Locale = TEXT("zh-CN");
	FText DialogueText;
	bool bGenerateAfterCreate = false;
	ECreateGenerationMode GenerationMode = ECreateGenerationMode::None;
};

FString GetDefaultClipName(const USoundWave* SoundWave)
{
	const FString SoundName = SoundWave ? SoundWave->GetName() : TEXT("Audio");
	return FString::Printf(TEXT("LSC_%s"), *SoundName);
}

FString GetDefaultPackagePath(const FAssetData& AssetData)
{
	const FString PackagePath = AssetData.PackagePath.ToString();
	return PackagePath.IsEmpty() ? TEXT("/Game") : PackagePath;
}

FText GetGenerationModeText(ECreateGenerationMode Mode)
{
	switch (Mode)
	{
	case ECreateGenerationMode::Rhubarb:
		return LOCTEXT("GenerationModeRhubarb", "Rhubarb");
	case ECreateGenerationMode::MFA:
		return LOCTEXT("GenerationModeMFA", "MFA");
	case ECreateGenerationMode::Ace:
		return LOCTEXT("GenerationModeAce", "NVIDIA ACE");
	default:
		return LOCTEXT("GenerationModeNone", "None");
	}
}

class SDreamLipSyncCreateClipDialog final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDreamLipSyncCreateClipDialog) {}
		SLATE_ARGUMENT(FCreateClipOptions, InitialOptions)
		SLATE_ARGUMENT(bool, StepByStep)
		SLATE_ARGUMENT(int32, SoundWaveCount)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Options = InArgs._InitialOptions;
		bStepByStep = InArgs._StepByStep;
		SoundWaveCount = InArgs._SoundWaveCount;

		GenerationItems =
		{
			MakeShared<ECreateGenerationMode>(ECreateGenerationMode::None),
			MakeShared<ECreateGenerationMode>(ECreateGenerationMode::Rhubarb),
			MakeShared<ECreateGenerationMode>(ECreateGenerationMode::MFA),
			MakeShared<ECreateGenerationMode>(ECreateGenerationMode::Ace)
		};
		SelectedGenerationItem = GenerationItems[0];

		ChildSlot
		[
			SNew(SBorder)
			.Padding(12.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
					.Text(this, &SDreamLipSyncCreateClipDialog::GetTitleText)
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				[
					SAssignNew(ContentBox, SBox)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 12.f, 0.f, 0.f)
				[
					BuildButtons()
				]
			]
		];

		RefreshContent();
	}

	void SetParentWindow(TSharedRef<SWindow> InWindow)
	{
		ParentWindow = InWindow;
	}

	bool WasAccepted() const
	{
		return bAccepted;
	}

	const FCreateClipOptions& GetOptions() const
	{
		return Options;
	}

private:
	FText GetTitleText() const
	{
		if (bStepByStep)
		{
			return FText::Format(LOCTEXT("StepTitle", "Create Dream Lip Sync Clip - Step {0} / 3"), FText::AsNumber(PageIndex + 1));
		}

		return LOCTEXT("FormTitle", "Create Dream Lip Sync Clip");
	}

	TSharedRef<SWidget> BuildTextRow(const FText& Label, TSharedRef<SWidget> ValueWidget) const
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 3.f, 10.f, 3.f)
			[
				SNew(STextBlock)
				.MinDesiredWidth(120.f)
				.Text(Label)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(0.f, 3.f)
			[
				ValueWidget
			];
	}

	TSharedRef<SWidget> BuildBasicPage()
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(SoundWaveCount > 1
					? FText::Format(LOCTEXT("MultiSoundWaveNotice", "{0} SoundWave assets selected. Asset Name is used only when one asset is selected; otherwise each clip uses LSC_<SoundWaveName>."), FText::AsNumber(SoundWaveCount))
					: LOCTEXT("SingleSoundWaveNotice", "Create one DreamLipSyncClip from the selected SoundWave."))
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildTextRow(LOCTEXT("AssetName", "Asset Name"),
					SNew(SEditableTextBox)
					.Text(FText::FromString(Options.AssetName))
					.IsEnabled(SoundWaveCount == 1)
					.OnTextChanged_Lambda([this](const FText& Text) { Options.AssetName = Text.ToString(); }))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildTextRow(LOCTEXT("PackagePath", "Package Path"),
					SNew(SEditableTextBox)
					.Text(FText::FromString(Options.PackagePath))
					.OnTextChanged_Lambda([this](const FText& Text) { Options.PackagePath = Text.ToString(); }))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildTextRow(LOCTEXT("Locale", "Locale"),
					SNew(SEditableTextBox)
					.Text(FText::FromString(Options.Locale))
					.OnTextChanged_Lambda([this](const FText& Text) { Options.Locale = Text.ToString(); }))
			];
	}

	TSharedRef<SWidget> BuildTranscriptPage()
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DialogueTextLabel", "Dialogue Text"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SMultiLineEditableTextBox)
				.Text(Options.DialogueText)
				.HintText(LOCTEXT("DialogueHint", "MFA requires transcript text. Rhubarb can use this as an optional dialog hint."))
				.OnTextChanged_Lambda([this](const FText& Text) { Options.DialogueText = Text; })
			];
	}

	TSharedRef<SWidget> BuildGenerationPage()
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildTextRow(LOCTEXT("GenerateAfterCreate", "Generate After Create"),
					SNew(SCheckBox)
					.IsChecked(Options.bGenerateAfterCreate ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Options.bGenerateAfterCreate = State == ECheckBoxState::Checked; }))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildTextRow(LOCTEXT("GenerationMethod", "Generation Method"), BuildGenerationCombo())
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("GenerationHelp", "None only creates the asset. MFA uses Dialogue Text. Rhubarb and ACE require their corresponding plugin settings to be configured."))
				.AutoWrapText(true)
			];
	}

	TSharedRef<SWidget> BuildGenerationCombo()
	{
		return SNew(SComboBox<TSharedPtr<ECreateGenerationMode>>)
			.OptionsSource(&GenerationItems)
			.InitiallySelectedItem(SelectedGenerationItem)
			.OnGenerateWidget_Lambda([](TSharedPtr<ECreateGenerationMode> Item)
			{
				return SNew(STextBlock).Text(GetGenerationModeText(Item.IsValid() ? *Item : ECreateGenerationMode::None));
			})
			.OnSelectionChanged_Lambda([this](TSharedPtr<ECreateGenerationMode> Item, ESelectInfo::Type)
			{
				if (Item.IsValid())
				{
					SelectedGenerationItem = Item;
					Options.GenerationMode = *Item;
					Options.bGenerateAfterCreate = Options.GenerationMode != ECreateGenerationMode::None;
					RefreshContent();
				}
			})
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return GetGenerationModeText(SelectedGenerationItem.IsValid() ? *SelectedGenerationItem : ECreateGenerationMode::None);
				})
			];
	}

	TSharedRef<SWidget> BuildForm()
	{
		return SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)[BuildBasicPage()]
				+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, 12.f)[BuildTranscriptPage()]
				+ SVerticalBox::Slot().AutoHeight()[BuildGenerationPage()]
			];
	}

	TSharedRef<SWidget> BuildButtons()
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(SSpacer)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Back", "Back"))
				.Visibility_Lambda([this]() { return bStepByStep && PageIndex > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
				.OnClicked_Lambda([this]()
				{
					PageIndex = FMath::Max(0, PageIndex - 1);
					RefreshContent();
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text_Lambda([this]() { return bStepByStep && PageIndex < 2 ? LOCTEXT("Next", "Next") : LOCTEXT("Create", "Create"); })
				.OnClicked(this, &SDreamLipSyncCreateClipDialog::OnPrimaryClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("Cancel", "Cancel"))
				.OnClicked_Lambda([this]()
				{
					CloseDialog();
					return FReply::Handled();
				})
			];
	}

	FReply OnPrimaryClicked()
	{
		if (bStepByStep && PageIndex < 2)
		{
			PageIndex = FMath::Min(2, PageIndex + 1);
			RefreshContent();
			return FReply::Handled();
		}

		if (!Validate())
		{
			return FReply::Handled();
		}

		bAccepted = true;
		CloseDialog();
		return FReply::Handled();
	}

	bool Validate() const
	{
		if (Options.PackagePath.IsEmpty() || !Options.PackagePath.StartsWith(TEXT("/Game")))
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("InvalidPackagePath", "Package Path must start with /Game."));
			return false;
		}

		if (SoundWaveCount == 1 && Options.AssetName.TrimStartAndEnd().IsEmpty())
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("InvalidAssetName", "Asset Name cannot be empty."));
			return false;
		}

		if (Options.bGenerateAfterCreate && Options.GenerationMode == ECreateGenerationMode::None)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("InvalidGenerationMode", "Choose a generation method or disable Generate After Create."));
			return false;
		}

		return true;
	}

	void RefreshContent()
	{
		if (!ContentBox.IsValid())
		{
			return;
		}

		if (!bStepByStep)
		{
			ContentBox->SetContent(BuildForm());
			return;
		}

		if (PageIndex == 0)
		{
			ContentBox->SetContent(BuildBasicPage());
		}
		else if (PageIndex == 1)
		{
			ContentBox->SetContent(BuildTranscriptPage());
		}
		else
		{
			ContentBox->SetContent(BuildGenerationPage());
		}
	}

	void CloseDialog()
	{
		if (TSharedPtr<SWindow> Window = ParentWindow.Pin())
		{
			Window->RequestDestroyWindow();
		}
	}

	FCreateClipOptions Options;
	TArray<TSharedPtr<ECreateGenerationMode>> GenerationItems;
	TSharedPtr<ECreateGenerationMode> SelectedGenerationItem;
	TSharedPtr<SBox> ContentBox;
	TWeakPtr<SWindow> ParentWindow;
	bool bAccepted = false;
	bool bStepByStep = false;
	int32 PageIndex = 0;
	int32 SoundWaveCount = 1;
};

bool ShowCreateDialog(const FCreateClipOptions& InitialOptions, bool bStepByStep, int32 SoundWaveCount, FCreateClipOptions& OutOptions)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(bStepByStep ? LOCTEXT("StepByStepWindowTitle", "Create LipSync Clip Step by Step") : LOCTEXT("FormWindowTitle", "Create LipSync Clip"))
		.ClientSize(FVector2D(620.f, bStepByStep ? 360.f : 560.f))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TSharedRef<SDreamLipSyncCreateClipDialog> Dialog = SNew(SDreamLipSyncCreateClipDialog)
		.InitialOptions(InitialOptions)
		.StepByStep(bStepByStep)
		.SoundWaveCount(SoundWaveCount);
	Dialog->SetParentWindow(Window);
	Window->SetContent(Dialog);

	FSlateApplication::Get().AddModalWindow(Window, nullptr);
	if (!Dialog->WasAccepted())
	{
		return false;
	}

	OutOptions = Dialog->GetOptions();
	return true;
}

TArray<USoundWave*> LoadSoundWaves(const FToolMenuContext& MenuContext)
{
	TArray<USoundWave*> SoundWaves;
	if (const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext))
	{
		for (const FAssetData& AssetData : Context->GetSelectedAssetsOfType(USoundWave::StaticClass()))
		{
			if (USoundWave* SoundWave = Cast<USoundWave>(AssetData.GetAsset()))
			{
				SoundWaves.Add(SoundWave);
			}
		}
	}
	return SoundWaves;
}

void InitOptionsFromSelection(const TArray<FAssetData>& SoundWaveAssets, FCreateClipOptions& OutOptions)
{
	if (SoundWaveAssets.IsEmpty())
	{
		OutOptions.AssetName = TEXT("LSC_Audio");
		OutOptions.PackagePath = TEXT("/Game");
		return;
	}

	const FAssetData& FirstAsset = SoundWaveAssets[0];
	OutOptions.PackagePath = GetDefaultPackagePath(FirstAsset);
	if (const USoundWave* SoundWave = Cast<USoundWave>(FirstAsset.GetAsset()))
	{
		OutOptions.AssetName = GetDefaultClipName(SoundWave);
	}
	else
	{
		OutOptions.AssetName = FString::Printf(TEXT("LSC_%s"), *FirstAsset.AssetName.ToString());
	}
}

bool RunGeneration(UDreamLipSyncClip* Clip, ECreateGenerationMode Mode, FString& OutMessage)
{
	switch (Mode)
	{
	case ECreateGenerationMode::Rhubarb:
		return FDreamLipSyncRhubarbGenerator::GenerateClipFromRhubarb(Clip, OutMessage);
	case ECreateGenerationMode::MFA:
		return FDreamLipSyncMfaGenerator::GenerateClipFromMfa(Clip, OutMessage);
	case ECreateGenerationMode::Ace:
		return FDreamLipSyncAceGenerator::GenerateClipFromAce(Clip, OutMessage);
	default:
		OutMessage = TEXT("Created.");
		return true;
	}
}

UDreamLipSyncClip* CreateClipAsset(USoundWave* SoundWave, const FString& DesiredAssetName, const FCreateClipOptions& Options)
{
	if (!SoundWave)
	{
		return nullptr;
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	FString PackageName;
	FString AssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(Options.PackagePath / DesiredAssetName, TEXT(""), PackageName, AssetName);

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return nullptr;
	}

	UDreamLipSyncClip* Clip = NewObject<UDreamLipSyncClip>(Package, UDreamLipSyncClip::StaticClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	if (!Clip)
	{
		return nullptr;
	}

	Clip->Sound = SoundWave;
	Clip->Locale = Options.Locale;
	Clip->DialogueText = Options.DialogueText;
	const float SoundDuration = SoundWave->GetDuration();
	if (SoundDuration > 0.f && SoundDuration < 100000.f)
	{
		Clip->Duration = SoundDuration;
	}
	if (SoundWave->AssetImportData)
	{
		Clip->SourceAudioFileOverride.FilePath = SoundWave->AssetImportData->GetFirstFilename();
	}

	FAssetRegistryModule::AssetCreated(Clip);
	Package->MarkPackageDirty();
	return Clip;
}

void ExecuteCreateClips(const FToolMenuContext& MenuContext, bool bStepByStep)
{
	const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext);
	if (!Context)
	{
		return;
	}

	const TArray<FAssetData> SoundWaveAssets = Context->GetSelectedAssetsOfType(USoundWave::StaticClass());
	if (SoundWaveAssets.IsEmpty())
	{
		return;
	}

	FCreateClipOptions InitialOptions;
	InitOptionsFromSelection(SoundWaveAssets, InitialOptions);

	FCreateClipOptions Options;
	if (!ShowCreateDialog(InitialOptions, bStepByStep, SoundWaveAssets.Num(), Options))
	{
		return;
	}

	TArray<UObject*> CreatedAssets;
	TArray<FString> Messages;
	for (const FAssetData& AssetData : SoundWaveAssets)
	{
		USoundWave* SoundWave = Cast<USoundWave>(AssetData.GetAsset());
		if (!SoundWave)
		{
			continue;
		}

		const FString DesiredAssetName = SoundWaveAssets.Num() == 1 ? Options.AssetName : GetDefaultClipName(SoundWave);
		UDreamLipSyncClip* Clip = CreateClipAsset(SoundWave, DesiredAssetName, Options);
		if (!Clip)
		{
			Messages.Add(FString::Printf(TEXT("%s: Failed to create clip."), *SoundWave->GetName()));
			continue;
		}

		CreatedAssets.Add(Clip);
		if (Options.bGenerateAfterCreate)
		{
			FString Message;
			RunGeneration(Clip, Options.GenerationMode, Message);
			Messages.Add(FString::Printf(TEXT("%s: %s"), *Clip->GetName(), *Message));
		}
		else
		{
			Messages.Add(FString::Printf(TEXT("%s: Created."), *Clip->GetName()));
		}
	}

	if (!CreatedAssets.IsEmpty())
	{
		FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().SyncBrowserToAssets(CreatedAssets);
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Join(Messages, TEXT("\n"))), LOCTEXT("CreateClipResultTitle", "Dream Lip Sync"));
}
}

namespace DreamLipSyncSoundWaveActions
{
void RegisterMenus(void* Owner)
{
	FToolMenuOwnerScoped OwnerScoped(Owner);
	UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(USoundWave::StaticClass());
	FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

	Section.AddDynamicEntry("DreamLipSyncSoundWaveActions", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
	{
		const UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>();
		if (!Context || Context->GetSelectedAssetsOfType(USoundWave::StaticClass()).IsEmpty())
		{
			return;
		}

		InSection.AddMenuEntry(
			"DreamLipSyncCreateClipStepByStep",
			LOCTEXT("CreateClipStepByStep", "Create LipSync Clip (Step by Step)..."),
			LOCTEXT("CreateClipStepByStepTooltip", "Create DreamLipSyncClip assets from selected SoundWave assets using a guided wizard."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.PlusCircle"),
			FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& MenuContext)
			{
				ExecuteCreateClips(MenuContext, true);
			}));

		InSection.AddMenuEntry(
			"DreamLipSyncCreateClipForm",
			LOCTEXT("CreateClipForm", "Create LipSync Clip (Form)..."),
			LOCTEXT("CreateClipFormTooltip", "Create DreamLipSyncClip assets from selected SoundWave assets using a single form."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
			FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& MenuContext)
			{
				ExecuteCreateClips(MenuContext, false);
			}));
	}));
}
}

#undef LOCTEXT_NAMESPACE
