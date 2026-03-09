using UnrealBuildTool;

public class InventoryCore : ModuleRules
{
	public InventoryCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"GameplayTags",
				"NetCore"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Engine",
				"InventoryUI"
			}
		);

		CircularlyReferencedDependentModules.AddRange(
			new string[]
			{
				"InventoryUI"
			}
		);
	}
}
