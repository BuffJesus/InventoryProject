# InventoryProject

A robust, spatial (grid-based) inventory system developed with **Unreal Engine 5.7**. This plugin provides a flexible framework for managing items in a 2D grid, supporting complex item shapes, stacking, and fragment-based item data.

### Key Features

- **Spatial Grid Inventory**: Manage items in a 2D grid with support for multi-tile items (dimensions).
- **Fragment-Based Item System**: Items are defined by manifests containing modular fragments, allowing for highly customizable item properties.
- **Dynamic UI**: Includes a responsive UI system with grid slots that reflect their state (occupied, unoccupied, selected, or grayed out).
- **Networking Support**: Designed with replication in mind, utilizing Fast Array Serialization for efficient inventory synchronization.
- **Flexible Hover Logic**: Intelligent hover detection that handles item dimensions, boundaries, and potential swapping/combining operations.
- **Gameplay Tag Integration**: Uses Unreal's Gameplay Tag system for item categorization and fragment identification.

### Technical Overview

- **`UINV_InventoryGrid`**: The core UI component for displaying and interacting with the spatial grid.
- **`UINV_InventoryComponent`**: An Actor Component that manages the inventory state and networking.
- **`FINV_ItemManifest`**: A data structure using `TInstancedStruct` to store modular item fragments.
- **`UINV_InventoryItem`**: The UObject representation of an item instance within the inventory.
- **`EINV_GridSlotState`**: An enumeration representing the visual and logical state of a grid tile.

### Requirements

- **Unreal Engine 5.7**
- **StructUtils Plugin** (for `TInstancedStruct`)
- **GameplayTags Plugin**

---
Developed with Unreal Engine 5
