> [!CAUTION]
> **This project is currently in early active development.** Features are subject to change, and experimental updates are frequent. Use with caution.

# sfm-editor

A high-performance **C++ 3D Editor** designed for visualizing, analyzing, and manipulating **Structure from Motion (SfM)** data and point clouds.

✨ **Key Features:**
* **Advanced SfM Visualization:** Load COLMAP binary/text, PLY, OBJ, and XYZ formats. Aspect-ratio-aware camera frustum rendering.
* **Interactive Manipulation:** Transform (Translate, Rotate, Scale) point clouds and camera poses using 3D Gizmos.
* **Camera Teleportation & Isolation:** Instantly teleport the viewport to any SfM camera's exact pose and FOV. Isolate specific 2D features in the 3D space.
* **Robust Tooling:** Box selection, screen-space camera picking, and a full Undo/Redo action history.

---

## 🛠️ Tech Stack
* **Language:** C++20
* **Graphics API:** OpenGL 4.6 (Core Profile)
* **GUI:** ImGui (Docking Branch) & ImGuizmo
* **Window Management:** GLFW & GLAD
* **Math:** GLM (OpenGL Mathematics)
* **Image Loading:** stb_image

---

## 🎹 Controls

### Viewport & Navigation
| Action | Control |
| :--- | :--- |
| **Rotate Camera** | Right Click (Hold) |
| **Pan Camera** | Middle Click (Hold) |
| **Zoom** | Mouse Scroll |
| **Focus Center** | `F` Key |
| **Movement (Free Fly)** | W, A, S, D, Q, E |
| **Speed Boost** | Left Shift |
| **Precision Slow** | Left Alt |

### Selection & Editing
| Action | Control |
| :--- | :--- |
| **Select Point/Camera** | Left Click |
| **Box Select** | Left Click + Drag |
| **Multi-Select / Toggle** | Hold `Ctrl` while clicking/dragging |
| **Select All** | `Ctrl + A` |
| **Delete Selected** | `Delete` |
| **Undo** | `Ctrl + Z` |
| **Redo** | `Ctrl + Y` or `Ctrl + Shift + Z` |

### Gizmo Modes
| Action | Control |
| :--- | :--- |
| **Hide Gizmo** | `Q` |
| **Translate Mode** | `W` |
| **Rotate Mode** | `E` |
| **Scale Mode** | `R` |

---

## License
This project is licensed under the Apache License 2.0. See the [LICENSE](LICENSE) and [NOTICE](NOTICE) files for more details.
