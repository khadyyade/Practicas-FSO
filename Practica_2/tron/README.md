# TRON Game Project

This is a TRON-style game implemented in C, featuring multiple phases of development that progressively add more functionality and complexity.

## Game Overview
Players control light cycles (called "trons") that leave trails as they move. The objective is to survive longer than your opponents by avoiding collisions with walls, trails, and other players.

## Project Phases

### Phase 1: Basic Multiplayer Setup
- Implementation of basic game mechanics
- Added support for multiple opponents (1-9)
- Introduced opponent variability in movement
- Basic logging functionality
- Added random delay for opponents

### Phase 2: Memory Management & Synchronization
- Added shared memory management
- Implemented semaphores for synchronization
- Improved process coordination
- Enhanced collision detection
- Better screen management with synchronized access

### Phase 3: Process Separation
- Split the program into separate executables
- Created independent opponent processes
- Improved communication between processes
- Enhanced memory management
- Better process lifecycle management

### Phase 4: Thread Integration
- Converted user movement to use threads
- Maintained process-based opponents
- Improved game responsiveness
- Added game timer display
- Better resource management

### Phase 5: Collision Enhancement
- Added collision messaging system
- Implemented visual trail modification
- Created backward/forward trail tracking
- Enhanced opponent behavior post-collision
- Visual feedback for collisions ('X' and 'O' markers)

### Phase 6: Special Ability
- Added special ability activation (E key)
- Implemented temporary invulnerability
- Added countdown display on player trail
- One-time use ability
- Visual feedback for ability duration

## Compilation
```bash
gcc -c winsuport2.c -o winsuport2.o
gcc tron6.c winsuport2.o memoria.o missatge.o -o tron6 -lcurses -lpthread
gcc oponent6.c winsuport2.o memoria.o missatge.o -o oponent6 -lcurses
```

## Execution
```bash
./tron6 num_opponents variability logfile [min_delay max_delay]
```

### Parameters
- num_opponents: Number of opponents (1-9)
- variability: Direction change frequency (0-3)
- logfile: Path to log file
- min_delay/max_delay: Optional delay range values

## Controls
- Arrow keys: Move player
- E: Activate special ability (Phase 6)
- Return: Exit game
