FLAME IN FREEFALL: NASA PSI SUPER INTELLIGENCE (SI) FIRE SAFETY DASHBOARD

Developer: Shiven Sood
Email: shivensoid@gmail.com
GitHub Repository: https://github.com/Shiven-Sood/NASA-WEB-APP-Project


1. PROJECT OVERVIEW AND PROBLEM STATEMENT

Fire behaves fundamentally differently in reduced gravity than it does on Earth:

- Spherical Microgravity Flames (0g): In the absence of gravity-driven buoyancy, hot air does not rise. Flames form floating diffusion spheres driven primarily by low-velocity cabin ventilation flows (under 5 cm/s).
- Low-Temperature Cool Flames and Reignition: Liquid fuel droplets in microgravity can radiative-quench from hot flames (approx. 2000 K) into invisible, low-temperature Cool Flames (500 to 800 K). These produce flammable partial-oxidation vapors capable of spontaneous reignition (observed in NASA FLEX and CFI-G investigations).
- Lunar Base and Mars Habitat Hazards (>= 30% O2, <= 10.2 psi): Future exploration habitats are designed to operate at reduced ambient pressure (55 to 70.3 kPa) and enriched oxygen concentrations (>= 30% O2) to minimize astronaut pre-breathe times prior to extravehicular activities (EVAs). This atmospheric combination significantly lowers ignition energy and accelerates flame spread rates (observed in NASA Saffire I-VI and BASS-II investigations).

While NASA's Physical Sciences Informatics (PSI) repository contains decades of experimental combustion data, manually parsing inconsistent raw spreadsheets is time-consuming, and standard browser JavaScript struggles to index and sort large scientific datasets without performance degradation.


2. ENGINEERING SOLUTION AND SYSTEM ARCHITECTURE

To resolve both the multi-format data bottleneck and browser computation limits, this project implements a 100% user-input-driven C++17 backend compiled to WebAssembly (.wasm) paired with a hardware-accelerated 3D/2D WebGL2 graphics pipeline:

- Universal .CSV and .XLSX Telemetry Parser: The application initializes with an empty state and relies entirely on user-supplied data. Users upload raw .CSV or binary .XLSX Excel spreadsheets of any layout (supporting both standard multi-column tables and vertical NASA parameter sheets such as DSI-S1 Solidification Plan.xlsx). All measurements are ingested using strict 64-bit IEEE 754 double precision.
- 3D Morton / Z-Order Spatial Curve Indexing: The engine normalizes three physical dimensions (Oxygen Concentration %, Ambient Pressure kPa, and Flame Spread Rate mm/s) onto a 1024x1024x1024 spatial grid and interleaves their bits into a single 32-bit unsigned integer Morton code, preserving 3D spatial locality within 1D contiguous memory.
- Sub-Millisecond C++ Sorting and Spatial Trees: The WebAssembly backend executes a non-comparative O(N) Base-256 Radix Sort benchmarked live against Hybrid Timsort (RUN = 32) and Divide-and-Conquer Merge Sort (O(N log N)). Simultaneously, it constructs a 3D KD-Tree, R-Tree bounding boxes, and a 64-Bucket Chained Geohash Table in memory.
- Interactive 3D Cube, 2D Orthographic Projections, and Point Inspector: The WebGL2 viewport renders high-precision scientific points (cyan for nominal < 30% O2 regimes; orange/red for >= 30% O2 Lunar hazard regimes) locked at 35 FPS. It includes smooth mouse-wheel zoom, one-click 2D orthographic plane projections (XY, XZ, YZ), dynamically scaled axis numbering, and a hover inspector displaying exact physical coordinates and hexadecimal Morton codes.
- Future Space Experiment Predictive Calculator: An integrated physics-informed simulation tool allows engineers to input hypothetical mission parameters (Oxygen %, Pressure kPa, Ventilation Flow cm/s, Gravity Regime, and Material Type) to calculate Predicted Flame Spread Rate, Limiting Oxygen Index (LOI), and Habitat Flammability Risk Scores (0 to 100), and inject simulated test points directly into the 3D WebGL2 viewport.


3. DATA STRUCTURES AND ALGORITHMS COMPLEXITY ANALYSIS

| Algorithm / Data Structure      | Best Time   | Avg / Worst Time | Space      | System Role                                                 |
|---------------------------------|-------------|------------------|------------|-------------------------------------------------------------|
| 3D Z-Order Bit-Interleaving     | O(N)        | O(N)             | O(1) aux   | Maps 3D (O2, Pressure, Spread) into a 32-bit spatial index  |
| Base-256 Radix Sort (4 Passes)  | O(4N) = O(N)| O(4N) = O(N)     | O(N + 256) | Primary linear-time spatial sort for GPU VBO upload         |
| Hybrid Timsort (RUN = 32)       | O(N)        | O(N log N)       | O(N)       | Exploits natural chronological runs in sensor telemetry     |
| Divide and Conquer Merge Sort   | O(N log N)  | O(N log N)       | O(N)       | Baseline O(N log N) benchmark timed via emscripten_get_now  |
| 3D KD-Tree                      | O(log N)    | O(N log N) build | O(N)       | Nearest-neighbor lookup for closest historical NASA test    |
| 64-Bucket Geohash Table         | O(1) insert | O(N / 64) lookup | O(N)       | Groups spatial flammability regimes for rapid filtering     |


4. USER GUIDE AND OPERATING INSTRUCTIONS

Step 1: Upload Telemetry Data (Panel 1)
Drag and drop any .CSV or .XLSX file (such as DSI-S1 Solidification Plan.xlsx or telemetry.csv included in the repository) into the upload box on the left panel, or click the box to browse local files.

Step 2: Inspect the 3D and 2D Phase-Space Viewport (Center Panel)
- Rotate (3D Mode): Click and drag the mouse across the canvas to rotate the 3D wireframe bounding cube and X/Y/Z axes.
- Zoom: Scroll the mouse wheel or use the +/- zoom controls (40% to 250%) to inspect dense data clusters.
- Switch to 2D Orthographic Views: Select "2D: O2 vs Pressure (XY)", "2D: O2 vs Spread (XZ)", or "2D: Pressure vs Spread (YZ)" in the top toolbar to view flat 2D scientific scatter plots.
- Inspect Individual Points: Hover the cursor over any data point to highlight it and view its exact Oxygen %, Pressure kPa, Spread Rate mm/s, and hexadecimal Morton code.

Step 3: Review C++ Benchmarks and Execute KD-Tree Queries (Panels 2 and 3)
- Panel 2 displays the total parsed records, average oxygen concentration, maximum spread rate, Lunar Base hazard alerts, and live sub-millisecond C++ sorting execution times.
- In Panel 3, enter a target Oxygen (%) and Pressure (kPa) and click "Run C++ 3D KD-Tree Nearest-Neighbor Search" to locate the closest historical experiment in C++ memory, or query the Super Intelligence (SI) text assistant regarding cool flames, lunar base risks, or smoke detection.

Step 4: Simulate Future Space Experiments (Panel 4)
Adjust Oxygen Concentration, Ambient Pressure, Ventilation Flow, Gravity Regime (0g, 0.16g, 0.38g, 1.0g), and Fuel Material in the Predictive Calculator. Click "Inject & Plot Simulated Experiment in 3D Cube" to append the simulated data point directly into the live WebGL2 visualization.

Step 5: Filter and Export Clean Datasets (Panels 1, 5, and 6)
Use the dropdown menus in Panel 1 to filter records by Mission Environment (ISS, Lunar Base, Martian Habitat, Crew Rover), Fire Safety Pillar (Prevention, Detection, Suppression), or specific NASA PSI Investigation. Click "Export Clean CSV" or "Export JSON" to download the processed dataset for external predictive modeling.


5. REPOSITORY STRUCTURE AND LOCAL COMPILATION

Directory Structure:
- include/sorting_engine.h     : 64-bit FireData structure and AlgorithmMetrics declarations
- include/telemetry_parser.h   : Universal CSV/XLSX telemetry parser interface
- src/main.cpp                 : WebGL2 shaders, 3D wireframe axes, zoom, and WebAssembly C-bridges
- src/sorting_engine.cpp       : Morton curve mapping, Radix Sort, Timsort, Merge Sort, and 3D KD-Tree
- src/telemetry_parser.cpp     : Multi-column and vertical spreadsheet numerical extractor
- shell.html                   : User interface template, 2D/3D HUD axis overlay, and Predictive Calculator
- dashboard.html               : Compiled Emscripten application entry point
- dashboard.js                 : Compiled WebAssembly JavaScript runtime loader
- dashboard.wasm               : Compiled C++17 WebAssembly binary

Recompiling from C++17 Source (Optional, requires Emscripten SDK):
.\emsdk\emsdk_env.ps1
em++ src/main.cpp src/sorting_engine.cpp src/telemetry_parser.cpp -I include -o dashboard.html -s WASM=1 -s USE_WEBGL2=1 -s EXPORTED_RUNTIME_METHODS="['ccall']" --shell-file shell.html


6. LIVE APPLICATION LINK

Access the live interactive web application here:
https://shiven-sood.github.io/NASA-WEB-APP-Project/dashboard.html

