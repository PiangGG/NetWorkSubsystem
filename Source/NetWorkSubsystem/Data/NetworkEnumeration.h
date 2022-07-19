// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EGameState : uint8 {
	ENone				UMETA(DisplayName = "None"),
	ELoadingScreen		UMETA(DisplayName = "Loading"),
	EStartup			UMETA(DisplayName = "Startup"),
	EMainMenu			UMETA(DisplayName = "Main Menu"),
	EMultiplayerHome	UMETA(DisplayName = "Multiplayer Home"),
	EMultiplayerJoin	UMETA(DisplayName = "Mulitplayer Join"),
	EMultiplayerHost	UMETA(DisplayName = "Multiplayer Host"),
	EMultiplayerInGame	UMETA(DisplayName = "Multiplayer In Game"),
	ETravelling			UMETA(DisplayName = "Travelling"),
};

/* ENUM TO TRACK INPUT STATES */
UENUM(BlueprintType)
enum class EInputMode : uint8 {
	EUIOnly				UMETA(DisplayName = "UI Only"),
	EUIAndGame			UMETA(DisplayName = "UI And Game"),
	EGameOnly			UMETA(DisplayName = "Game Only"),
};