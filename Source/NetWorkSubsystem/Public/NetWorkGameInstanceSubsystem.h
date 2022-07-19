// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NetWorkSubsystem/Data/NetworkStructure.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NetWorkSubsystem/Data/NetworkEnumeration.h"
#include "NetWorkGameInstanceSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKSUBSYSTEM_API UNetWorkGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	//Constructor
	UNetWorkGameInstanceSubsystem(const FObjectInitializer& ObjectInitializer);

	virtual void Init();
	/* Widget references */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Manager")
	TSubclassOf<class UUserWidget> cMainMenu;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Manager")
	TSubclassOf<class UUserWidget> cMPHome;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Manager")
	TSubclassOf<class UUserWidget> cMPJoin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Manager")
	TSubclassOf<class UUserWidget> cMPHost;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Manager")
	TSubclassOf<class UUserWidget> cLoadingScreen;

	/* STATE CHANGES */
	UFUNCTION(BlueprintCallable, Category = "Platformer Game Instance")
    void ChangeState(EGameState newState);
	
	//Get current game state
	UFUNCTION(BlueprintCallable, Category = "Platformer Game Instance")
    EGameState GetCurrentGameState();

	/* Input mode handling */
	//function for setting the input mode
	UFUNCTION(BlueprintCallable, Category = "Platformer Game Instance")
    void SetInputMode(EInputMode newInputMode, bool bShowMouseCursor);

	//Current Input Mode
	UPROPERTY(BlueprintReadOnly, Category = "Platformer Game Instance")
	EInputMode CurrentInputMode;
	//are we displaying the mouse cursor
	UPROPERTY(BlueprintReadOnly, Category = "Platformer Game Instance")
	bool bIsShowingMouseCursor;

	/* Online Session Interface */

	//Shared pointer for holding session settings
	TSharedPtr<class FOnlineSessionSettings> SessionSettings;

	/* SESSION CREATION */
	//function for hosting a session from blueprints
	UFUNCTION(BlueprintCallable, Category = "Session Management")
	void HostGame(bool bIsLAN, int32 MaxNumPlayers, TArray<FBlueprintSessionSetting> sessionSettings);

	//c++ function for hosting a session
	bool HostSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, bool bIsLAN, int32 MaxNumPlayers, TMap<FString, FOnlineSessionSetting> SettingsMap);

	//delegate function which will be called when session is created
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	//delegate to be called when session is created
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;

	//delegate handle retained after registering delegate
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;

	/* STARTING A SESSION */

	//delegate function which will be called when session is started
	void OnStartOnlineGameComplete(FName SessionName, bool bWasSuccessful);

	//delegate to be called when session is started
	FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;

	//delegate handle retained after registering delegate
	FDelegateHandle OnStartSessionCompleteDelegateHandle;

	/* FINDING SESSIONS */

	//Array for holding our search results
	UPROPERTY(BlueprintReadOnly, Category = "Session Management")
	TArray<FBlueprintSearchResult> searchResults;

	//booleans for keeping track of search status
	UPROPERTY(BlueprintReadOnly, Category = "Session Management")
	bool bHasFinishedSearchingForGames;
	UPROPERTY(BlueprintReadOnly, Category = "Session Management")
	bool bSearchingForGames;

	//shared pointer to our c++ native search results
	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	//blueprint function for finding games
	UFUNCTION(BlueprintCallable, Category = "Session Management")
	void FindGames(bool bIsLAN);

	//c++ function for finding sessions
	void FindSessions(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, bool bIsLAN);

	//delegate function called when FindSessions completes
	void OnFindSessionsComplete(bool bWasSuccessful);

	//Delegate for OnFindSessionsComplete
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;

	//delegate handle for OnFindSessionsComplete
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;


	/* JOIN SESSIONS */
	//Blueprint function for joining a session
	UFUNCTION(BlueprintCallable, Category = "Session Management")
	void JoinGame(FBlueprintSearchResult result);

	//c++ function for joining the session
	bool JoinSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, const FOnlineSessionSearchResult& SearchResult);

	//delegate function which will be called when a session has been joined
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	//delegate for binding our function
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;

	//delegate handle for OnJoinSessionCompleteDelegate
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;

	/* UPDATING SESSION */
	//function for getting the current value of a special setting for the active session
	UFUNCTION(BlueprintCallable, Category = "Session Management")
	FString GetSessionSpecialSettingString(FString key);

	//function for creating a new or updating an existing special setting on the active session
	//host only
	UFUNCTION(BlueprintCallable, Category = "Session Management")
	void SetOrUpdateSessionSpecialSettingString(FBlueprintSessionSetting newSetting);

	//delegate function which will be called after update session completes
	void OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful);

	//delegate which will be called after updating a session
	FOnUpdateSessionCompleteDelegate OnUpdateSessionCompleteDelegate;

	//delegate handle for OnUpdateSessionCompleteDelegate
	FDelegateHandle OnUpdateSessionCompleteDelegateHandle;

	/* DESTROYING A SESSION / LEAVING GAME */
	//Blueprint function for leaving game
	UFUNCTION(BlueprintCallable, Category = "Session Management")
	void LeaveGame();

	//delegate function which will be called after leaving session
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	//delegate which will be called after destroying a session
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;

	//delegate handle for OnDestroySessionCompleteDelegate
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;

	/* HANDLE NETWORK ERRORS */
	void HandleNetworkError(UWorld *World, UNetDriver *NetDriver, ENetworkFailure::Type FailureType, const FString & ErrorString);

	private:
	//currently displayed widget
	UUserWidget *currentWidget;
	//our current game state
	EGameState currentState;

	//function for entering a state
	void EnterState(EGameState newState);
	//function for leaving a state
	void LeaveState();

	UFUNCTION(BlueprintCallable)
	FString ReturnPath();
};
