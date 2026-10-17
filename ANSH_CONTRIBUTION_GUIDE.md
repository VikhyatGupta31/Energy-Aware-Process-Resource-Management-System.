# Ansh Part 3: Contribution and Review Guide

This note helps Ansh describe and review his part of the Energy-Aware Process and Resource Management System. It is not proof of authorship by itself. Keep only statements that match the work Ansh actually completed in the team's Git history and source files.

## Contribution areas shown by the supplied source snapshot

- `energy_estimator.py`: formula-based energy estimate and schedule total (marked Owner: Ansh in the supplied project copy).
- `app.py`: Flask dashboard starter (marked Owner: Ansh; currently a Phase-I sample-data prototype in the supplied ZIP).

## Overall flow

```mermaid
flowchart LR
    A[Scheduling/resource inputs] --> B[Energy estimator]
    B --> C[Estimated energy results]
    C --> D[Dashboard API]
    D --> E[Dashboard charts]
```

In the supplied ZIP, the dashboard API uses sample values, so the last connection is a demonstration flow rather than a verified live database flow.

## Short explanation Ansh can use

> I worked on the formula-based energy estimate and the Flask dashboard prototype. The estimator calculates a software-level estimate from resource inputs. In the ZIP copy I reviewed, the dashboard uses sample data, so I will describe it as a prototype unless the current Git version includes and verifies live database integration.

Edit this explanation if it does not match Ansh's actual work.

## Separate documentation commits

These files can be reviewed and committed independently, if the team wants separate commits:

1. `docs/ANSH_ENERGY_ESTIMATOR.md` — `docs: explain energy estimator flow`
2. `docs/ANSH_DASHBOARD_FLOW.md` — `docs: describe Flask dashboard prototype`
3. `docs/ANSH_CONTRIBUTION_GUIDE.md` — `docs: add Ansh contribution review guide`

A documentation commit records documentation work; it does not increase or replace Ansh's code contribution. He should commit only work he wrote or reviewed, and must not claim someone else's code. The supplied Downloads ZIP is an extracted copy and has no `.git` metadata; run Git commands from the actual cloned repository.
