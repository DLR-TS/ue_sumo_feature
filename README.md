# GEMSTAR UE_SUMO_Plugin

[![DLR-TS](https://img.shields.io/badge/developed%20by-DLR%20TS-orange.svg?style=flat&colorA=E1523D&colorB=007D8A)](https://www.dlr.de/en/ts/about-us/the-institute-of-transportation-systems)

This plugin adds [SUMO](https://github.com/eclipse-sumo/sumo) in version 1.22.0 to GEMSTAR.
SUMO simulates a scenario and sends information to GEMSTAR.

## Input

- SUMO simulation configuration (.sumocfg)
- additional configuration files

## Output

- Vehicle information
- Traffic Light Data

# Getting started

First clone the GEMSTAR Interfaces plugin inside the Plugins directory.

Add the plugin to your Unreal project:

```sh
mkdir Plugins/GameFeatures
cd Plugins/GameFeatures

git clone https://github.com/DLR-TS/ue_sumo_feature.git
```
or:
```sh
git submodule add https://github.com/DLR-TS/ue_sumo_feature.git
```

Convert an OpenDRIVE to a SUMO map to run with GEMSTAR:

```
netconvert --opendrive-files Map.xodr --offset.disable-normalization true --proj.rotate 270 -o Map.net.xml
```

# Test

![SUMO Configuration](Documentation/Configuration.png)

**Path** SUMO configuration file (.sumocfg)\
**Status** Status of library [Unset, Configured, Running, Done]\
**Coordinate System Offset** Additional offset (if needed)

---------------
**Contact:** opensource-ts@dlr.de

This project uses [Eclipse SUMO](https://github.com/eclipse-sumo/sumo), which is licensed under the Eclipse Public License 2.0.

This project uses the Open Simulation Interface ([OSI](https://github.com/OpenSimulationInterface/open-simulation-interface)), which is licensed under the
Mozilla Public License 2.0.
