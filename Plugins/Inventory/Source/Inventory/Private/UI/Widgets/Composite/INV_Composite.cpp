// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Composite/INV_Composite.h"
#include "Blueprint/WidgetTree.h"

void UINV_Composite::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (UINV_Composite_Base* Composite = Cast<UINV_Composite_Base>(Widget); IsValid(Composite))
		{
			Children.Add(Composite);
			Composite->Collapse();
		}
	});
}

void UINV_Composite::ApplyFunction(FuncType Function)
{
	for (auto& Child : Children)
	{
		Child->ApplyFunction(Function);
	}
}

void UINV_Composite::Collapse()
{
	for (auto& Child : Children)
	{
		Child->Collapse();
	}
}
