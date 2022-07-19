// Fill out your copyright notice in the Description page of Project Settings.


#include "NetWorkGameInstanceSubsystem.h"
#include "OnlineSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Online.h"
#include "Misc/Paths.h"

UNetWorkGameInstanceSubsystem::UNetWorkGameInstanceSubsystem(const FObjectInitializer& ObjectInitializer)
{
	//initial state is None 
	currentState = EGameState::ENone;
	//current widget is nothing
	currentWidget = nullptr;
        
	/* BIND FUNCTIONS FOR SESSION MANAGEMENT */

	//Create
	OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UNetWorkGameInstanceSubsystem::OnCreateSessionComplete);

	//Start
	OnStartSessionCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &UNetWorkGameInstanceSubsystem::OnStartOnlineGameComplete);

	//Find
	OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UNetWorkGameInstanceSubsystem::OnFindSessionsComplete);

	//Join
	OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UNetWorkGameInstanceSubsystem::OnJoinSessionComplete);

	//Update
	OnUpdateSessionCompleteDelegate = FOnUpdateSessionCompleteDelegate::CreateUObject(this, &UNetWorkGameInstanceSubsystem::OnUpdateSessionComplete);

	//Destroy
	OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UNetWorkGameInstanceSubsystem::OnDestroySessionComplete);


	//FPaths::ProjectPluginsDir()+TEXT("NetWorkSubsystem/Content/WBP/")+TEXT("JoinGameScreen/W_MultiplayerJoinGameMenu.W_MultiplayerJoinGameMenu_C'")
	//FPaths::ProjectPluginsDir();
	
	
}

void UNetWorkGameInstanceSubsystem::Init()
{
	cMainMenu = LoadClass<UUserWidget>(NULL, TEXT("WidgetBlueprint'/NetWorkSubsystem/WBP/W_MainMenu.W_MainMenu_C'"));
	cMPHome = LoadClass<UUserWidget>(NULL, TEXT("WidgetBlueprint'/NetWorkSubsystem/WBP/W_MultiplayerHome.W_MultiplayerHome_C'"));
	cMPJoin = LoadClass<UUserWidget>(NULL, TEXT("WidgetBlueprint'/NetWorkSubsystem/WBP/JoinGameScreen/W_MultiplayerJoinGameMenu.W_MultiplayerJoinGameMenu_C'"));
	cMPHost = LoadClass<UUserWidget>(NULL, TEXT("WidgetBlueprint'/NetWorkSubsystem/WBP/W_HostGameMenu.W_HostGameMenu_C'"));
	cLoadingScreen = LoadClass<UUserWidget>(NULL, TEXT("WidgetBlueprint'/NetWorkSubsystem/WBP/W_LoadingScreen.W_LoadingScreen_C'"));
}

void UNetWorkGameInstanceSubsystem::ChangeState(EGameState newState)
{
	Init();
	if (newState != currentState) {
		LeaveState();
		EnterState(newState);
	}
}

EGameState UNetWorkGameInstanceSubsystem::GetCurrentGameState()
{
	return currentState;
}

void UNetWorkGameInstanceSubsystem::SetInputMode(EInputMode newInputMode, bool bShowMouseCursor)
{
	switch (newInputMode) {
	case EInputMode::EUIOnly: {
			GetWorld()->GetFirstPlayerController()->SetInputMode(FInputModeUIOnly());
			break;
	}
	case EInputMode::EUIAndGame: {
			GetWorld()->GetFirstPlayerController()->SetInputMode(FInputModeGameAndUI());
			break;
	}
	case EInputMode::EGameOnly:
		{
			if (GetWorld()->GetFirstPlayerController())
			{
				GetWorld()->GetFirstPlayerController()->SetInputMode(FInputModeGameOnly());
			} 
			break;
		}
	}
	
	if ( GetWorld()->GetFirstPlayerController())
	{
		GetWorld()->GetFirstPlayerController()->bShowMouseCursor = bShowMouseCursor;
	}
	CurrentInputMode = newInputMode;
	bIsShowingMouseCursor = bShowMouseCursor;
}

void UNetWorkGameInstanceSubsystem::HostGame(bool bIsLAN, int32 MaxNumPlayers,
	TArray<FBlueprintSessionSetting> sessionSettings)
{
	//get the online subsystem
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	//if OnlineSub is valid
	if (OnlineSub) {
		//get unique player id, we use 0 since we dont allow multiple players per client
		TSharedPtr<const FUniqueNetId> pid = OnlineSub->GetIdentityInterface()->GetUniquePlayerId(0);

		//create the special settings map
		TMap<FString, FOnlineSessionSetting> SpecialSettings = TMap<FString, FOnlineSessionSetting>();

		//loop through any provided settings and add them to special settings map
		for (auto &setting : sessionSettings) {
			//create a new setting
			FOnlineSessionSetting newSetting;

			//set its data to the provided value
			newSetting.Data = setting.value;

			//ensure the setting is advertised over the network
			newSetting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineService;

			//add setting to the map with the specified key
			SpecialSettings.Add(setting.key, newSetting);
		}

		//Change the state to loading screen while attempting to host the game
		ChangeState(EGameState::ELoadingScreen);

		//host the session
		HostSession(pid, GameSessionName, bIsLAN, MaxNumPlayers, SpecialSettings);
	}
}

bool UNetWorkGameInstanceSubsystem::HostSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, bool bIsLAN,
	int32 MaxNumPlayers, TMap<FString, FOnlineSessionSetting> SettingsMap)
{
        IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

        if (OnlineSub) {
                IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
                if (Sessions.IsValid() && UserId.IsValid()) {
                        SessionSettings = MakeShareable(new FOnlineSessionSettings());
                        SessionSettings->bIsLANMatch = bIsLAN;
                        SessionSettings->bUsesPresence = true;
                        SessionSettings->NumPublicConnections = MaxNumPlayers;
                        SessionSettings->NumPrivateConnections = 0;
                        SessionSettings->bAllowInvites = true;
                        SessionSettings->bAllowJoinInProgress = true;
                        SessionSettings->bShouldAdvertise = true;
                        SessionSettings->bAllowJoinViaPresence = true;
                        SessionSettings->bAllowJoinViaPresenceFriendsOnly = false;

                        SessionSettings->Set(SETTING_MAPNAME, FString("Map_SandBox"), EOnlineDataAdvertisementType::ViaOnlineService);

                        for (auto &setting : SettingsMap) {
                                SessionSettings->Settings.Add(FName(*setting.Key), setting.Value);
                        }
                        OnCreateSessionCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
                        return Sessions->CreateSession(*UserId, SessionName, *SessionSettings);
                }
        }
        return false;
}

void UNetWorkGameInstanceSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub) {
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid()) {
			
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
			
			if (bWasSuccessful) {
				OnStartSessionCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegate);

				Sessions->StartSession(SessionName);
			}
		}
	}
}

void UNetWorkGameInstanceSubsystem::OnStartOnlineGameComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub) {
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid()) {
			Sessions->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegateHandle);
		}
	}

	if (bWasSuccessful) {
		UGameplayStatics::OpenLevel(GetWorld(), "Map_SandBox", true, "listen");

		ChangeState(EGameState::ETravelling);
	}
}

void UNetWorkGameInstanceSubsystem::FindGames(bool bIsLAN)
{
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();
	bHasFinishedSearchingForGames = false;
	bSearchingForGames = false;
	searchResults.Empty();

	if (OnlineSub) {
		TSharedPtr<const FUniqueNetId> pid = OnlineSub->GetIdentityInterface()->GetUniquePlayerId(0);

		FindSessions(pid, GameSessionName, bIsLAN);
	}
}

void UNetWorkGameInstanceSubsystem::FindSessions(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, bool bIsLAN)
{
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub) {
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid() && UserId.IsValid()) {
			SessionSearch = MakeShareable(new FOnlineSessionSearch());
			SessionSearch->bIsLanQuery = bIsLAN;
			SessionSearch->MaxSearchResults = 100000000;
			SessionSearch->PingBucketSize = 50;
			SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

			TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SessionSearch.ToSharedRef();

			OnFindSessionsCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

			bSearchingForGames = true;

			Sessions->FindSessions(*UserId, SearchSettingsRef);
		}
	}
	else {
		OnFindSessionsComplete(false);
	}
}

void UNetWorkGameInstanceSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub) {
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid()) {
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		}
	}

	if (bWasSuccessful) {
		for (auto &result : SessionSearch->SearchResults) {
			FBlueprintSearchResult newresult = FBlueprintSearchResult(result);
			searchResults.Add(newresult);
		}
	}

	bHasFinishedSearchingForGames = true;
	bSearchingForGames = false;
}

void UNetWorkGameInstanceSubsystem::JoinGame(FBlueprintSearchResult result)
{
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub) {
		TSharedPtr<const FUniqueNetId> pid = OnlineSub->GetIdentityInterface()->GetUniquePlayerId(0);
		JoinSession(pid, GameSessionName, result.result);
	}
}

bool UNetWorkGameInstanceSubsystem::JoinSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName,
	const FOnlineSessionSearchResult& SearchResult)
{
	bool bSuccessful = false;

	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub) {
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid() && UserId.IsValid()) {
			OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
			bSuccessful = Sessions->JoinSession(*UserId, SessionName, SearchResult);
		}
	}
	return bSuccessful;
}

void UNetWorkGameInstanceSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub) {
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid()) {
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);


			APlayerController *const PlayerController = GetWorld()->GetFirstPlayerController();//GetFirstLocalPlayerController();

			FString TravelURL;

			if (PlayerController && Sessions->GetResolvedConnectString(SessionName, TravelURL)) {
				PlayerController->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
				ChangeState(EGameState::ETravelling);
			}
		}
	}
}

FString UNetWorkGameInstanceSubsystem::GetSessionSpecialSettingString(FString key)
{
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub) {
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid()) {
			FOnlineSessionSettings *settings = Sessions->GetSessionSettings(GameSessionName);

			if (settings) {
				if (settings->Settings.Contains(FName(*key))) {
					FString value;
					settings->Settings[FName(*key)].Data.GetValue(value);

					return value;
				}
				else {
					return FString("INVALID KEY");
				}
			}
		}
		else {
			return FString("NO SESSION!");
		}
	}
	return FString("NO ONLINE SUBSYSTEM");
}

void UNetWorkGameInstanceSubsystem::SetOrUpdateSessionSpecialSettingString(FBlueprintSessionSetting newSetting)
{
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub) {
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid()) {
			FOnlineSessionSettings *settings = Sessions->GetSessionSettings(GameSessionName);

			if (settings) {
				if (settings->Settings.Contains(FName(*newSetting.key))) {
					
					settings->Settings[FName(*newSetting.key)].Data = newSetting.value;
				}
				else { 
					FOnlineSessionSetting setting;

					setting.Data = newSetting.value;
					setting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineService;
					
					settings->Settings.Add(FName(*newSetting.key), setting);
				}

				OnUpdateSessionCompleteDelegateHandle = Sessions->AddOnUpdateSessionCompleteDelegate_Handle(OnUpdateSessionCompleteDelegate);

				Sessions->UpdateSession(GameSessionName, *settings, true);
			}
		}
	}
}

void UNetWorkGameInstanceSubsystem::OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub) {
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid()) {
			Sessions->ClearOnUpdateSessionCompleteDelegate_Handle(OnUpdateSessionCompleteDelegateHandle);
		}
	}   
}

void UNetWorkGameInstanceSubsystem::LeaveGame()
{
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub) {
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid()) {
			OnDestroySessionCompleteDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);
			Sessions->DestroySession(GameSessionName);
		}
	}
}

void UNetWorkGameInstanceSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem *OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub) {
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid()) {
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);

		}
	}
	if (bWasSuccessful) {
		UGameplayStatics::OpenLevel(GetWorld(), "Map_MainMenu", true);
		ChangeState(EGameState::ETravelling);
	}
}

void UNetWorkGameInstanceSubsystem::HandleNetworkError(UWorld* World, UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	LeaveGame();
}

void UNetWorkGameInstanceSubsystem::EnterState(EGameState newState)
{
	 //set the current state to newState
    currentState = newState;

    switch (currentState)
	{
	    case EGameState::ELoadingScreen:
	    	{
	            //create the widget
	            currentWidget = CreateWidget<UUserWidget>(GetWorld()->GetFirstPlayerController(), cLoadingScreen);
	            //add the widget to the viewport
	          
	            if (currentWidget)
	            {
	                currentWidget->AddToViewport();
	                SetInputMode(EInputMode::EUIOnly, true);
	            }
	            //go to the appropriate input mode
	           
	            break;
	    }
	    case EGameState::EMainMenu:
	    	{
	            //create the widget
	            currentWidget = CreateWidget<UUserWidget>(GetWorld()->GetFirstPlayerController(), cMainMenu);
	            if (currentWidget)
	            {
	            //add the widget to the viewport
	            currentWidget->AddToViewport();

	            //go to the appropriate input mode
	            SetInputMode(EInputMode::EUIOnly, true);     
	            }
	              
	            break;
	    }
	    case EGameState::EMultiplayerHome:
	    	{
	            //create the widget
	            currentWidget = CreateWidget<UUserWidget>(GetWorld()->GetFirstPlayerController(), cMPHome);
	            if (currentWidget)
	            {
	                    //add the widget to the viewport
	                    currentWidget->AddToViewport();

	                    //go to the appropriate input mode
	                    SetInputMode(EInputMode::EUIOnly, true);      
	            }
	            
	            break;
	    	}

	    case EGameState::EMultiplayerJoin:
	    	{
	            //create the widget
	            currentWidget = CreateWidget<UUserWidget>(GetWorld()->GetFirstPlayerController(), cMPJoin);
	            //add the widget to the viewport
	            if (true)
	            {
	                    currentWidget->AddToViewport();

	                    //go to the appropriate input mode
	                    SetInputMode(EInputMode::EUIOnly, true);     
	            }
	            
	            break;
			}
	    case EGameState::EMultiplayerHost:
	    	{
	            //create the widget
	            currentWidget = CreateWidget<UUserWidget>(GetWorld()->GetFirstPlayerController(), cMPHost);
	            if (true)
	            {
	                    //add the widget to the viewport
	                    currentWidget->AddToViewport();

	                    //go to the appropriate input mode
	                    SetInputMode(EInputMode::EUIOnly, true);   
	            }
	            break;
	    	}
	    case EGameState::EMultiplayerInGame:
	    	{
	            SetInputMode(EInputMode::EGameOnly, false);
	            break;
			}
	    case EGameState::ETravelling:
	    	{
	            SetInputMode(EInputMode::EUIOnly, false);
	            break;
			}
	    case EGameState::ENone:
	    	{
	            break;
			}
	    default:
	    	{
	            break;
			}
    }
}

void UNetWorkGameInstanceSubsystem::LeaveState()
{
	switch (currentState) {
	case EGameState::ELoadingScreen: {
		
	}
	case EGameState::EMainMenu: {
		
	}
	case EGameState::EMultiplayerHome: {
		
	}
	case EGameState::EMultiplayerJoin: {
		
	}
	case EGameState::EMultiplayerHost: {
			if (currentWidget) {
				currentWidget->RemoveFromViewport();
				currentWidget = nullptr;
			}
			break;
	}
	case EGameState::EMultiplayerInGame: {
			break;
	}
	case EGameState::ETravelling: {
			break;
	}
	case EGameState::ENone: {
			break;
	}
	default: {
			break;
	}
	}
	EnterState(EGameState::ENone);
}

FString UNetWorkGameInstanceSubsystem::ReturnPath()
{
	
	return *("WidgetBlueprint'"+FPaths::ProjectPluginsDir()+"NetWorkSubsystem/Content/NetWorkSubsystem/WBP/W_MainMenu.W_MainMenu_C'");
}
