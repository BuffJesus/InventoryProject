using UnrealBuildTool;

public class InventoryUI : ModuleRules
{
	public InventoryUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"InventoryCore"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
				"SlateCore",
				"GameplayTags",
				"UMG",
				"EnhancedInput",
				"NetCore"
			}
		);
	}
}
