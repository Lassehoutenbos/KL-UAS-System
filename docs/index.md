---
layout: default
title: Home
---

# KL UAS System

An educational unmanned aircraft system (UAS) project that explores custom airframe design, autonomous flight software, and an operator-focused ground control station.

## Quick links

- [Repository home]({{ site.github.repository_url }})
- [Ground control station overview](ground-control-station.html)
- [Hardware and parts breakdown](hardware.html)
- [Code documentation](https://github.com/{{ site.github.repository_nwo }}/blob/main/Docs/CodeDocumentation.md)
- [Mission logs](https://github.com/{{ site.github.repository_nwo }}/tree/main/Docs/Logs)

## Project highlights

### Airframe and propulsion

The KL UAS platform is built around a 10-inch quadcopter configuration powered by four **Emax GT2218 1100KV** motors and folding Gemfan F1051-3 propellers. The drivetrain is paired with a SpeedyBee F7 V3 flight controller running INAV firmware, providing stable flight characteristics while supporting autonomous mission profiles.

### Sensing and situational awareness

Integrated **optical flow** and **LiDAR** sensors enhance close-quarters positioning, while a Foxeer M10Q 250 GPS with compass supports navigation at range. The airframe can lift substantial payloads, enabling applications that require extended endurance or specialized mission equipment.

### Operator experience

The project includes a purpose-built ground control station featuring dedicated hardware controls, telemetry displays, and ADS-B awareness to keep operators informed of nearby air traffic in real time. Explore the detailed layout on the [ground control station page](ground-control-station.html).

## Explore more

- Browse the [datasheets library](https://github.com/{{ site.github.repository_nwo }}/tree/main/Docs/Datasheets) for the sensors and components used in the build.
- Review procurement details in the [bill of materials](https://github.com/{{ site.github.repository_nwo }}/blob/main/Docs/Bestellen/Bestellijst.md).
- Check the [3D design files](https://github.com/{{ site.github.repository_nwo }}/tree/main/3D%20files) for printable hardware and mounts.

## Getting involved

Contributions that improve documentation, share flight logs, or enhance the ground station firmware are welcome. Please open an issue or pull request on GitHub with your ideas.

---

<small>
For GitHub Pages deployments, configure the site to build from the `docs/` folder on the `main` branch. Once enabled, the published site will be available at `https://&lt;your-github-username&gt;.github.io/{{ site.github.project_title }}/`.
</small>
