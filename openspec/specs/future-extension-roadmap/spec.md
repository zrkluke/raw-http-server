# Purpose

Define the discoverable, bounded roadmap for post-course extensions of the
three-language HTTP learning project.

## Requirements

### Requirement: Discoverable future-extension roadmap
The repository SHALL provide `docs/roadmap.md` as the detailed entry point for
post-course extensions, and the README SHALL link to it without duplicating the
roadmap content.

#### Scenario: Learner looks for next steps
- **WHEN** a learner reads the README after the completed course map
- **THEN** the learner can follow a link to the future-extension roadmap

### Requirement: Bounded extension planning
The roadmap SHALL distinguish completed course behavior from exploratory future
directions and SHALL not represent an extension as implemented or approved
work.

#### Scenario: Learner reads an unimplemented extension
- **WHEN** a learner reads a roadmap item
- **THEN** the item identifies its learning purpose and states that a separate
  OpenSpec change is required before implementation

### Requirement: Persistent-connection priority
The roadmap SHALL list HTTP/1.1 persistent connections as the recommended
first extension and SHALL explain the relationship between message framing,
leftover bytes, and connection lifetime.

#### Scenario: Learner evaluates the first extension
- **WHEN** a learner reads the persistent-connection item
- **THEN** the learner can identify sequential requests on one TCP connection,
  exact body consumption, and no-EOF response behavior as acceptance goals

### Requirement: Ranked follow-on directions
The roadmap SHALL list handler boundaries, static file serving, reverse
proxying, concurrency comparison, and an HTTP/2 frame experiment as later,
separately-scoped directions.

#### Scenario: Learner chooses a later topic
- **WHEN** a learner examines the ranked follow-on directions
- **THEN** the learner can identify each topic's learning focus without
  inferring that it belongs to the completed Steps 1–9 scope
