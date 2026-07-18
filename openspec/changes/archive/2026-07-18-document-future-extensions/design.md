## Context

Steps 1–9 are complete and `main` is the stable course result.  The project
already keeps detailed learning material under `docs/`, while the README is a
concise entry point.  Future extension ideas need to be visible without being
mistaken for implemented scope or active work.

## Goals / Non-Goals

**Goals:**

- Provide a durable, discoverable roadmap for post-course learning.
- Explain why persistent HTTP/1.1 connections are the recommended first
  extension of the existing byte-stream and framing work.
- Preserve the existing rule that each implementation effort receives its own
  OpenSpec change, branch, review, and immutable reference when appropriate.

**Non-Goals:**

- Implement keep-alive, routing, static files, reverse proxying, concurrency,
  or HTTP/2.
- Change the completed Steps 1–9 contract, tags, or verification results.
- Promise a delivery sequence or treat roadmap items as approved work.

## Decisions

### Put the roadmap in `docs/` and link it from README

The roadmap belongs in `docs/roadmap.md`, with one short README link. This
keeps the project entry point compact while making the detailed rationale easy
to find.

Alternative considered: place all future ideas in README. Rejected because it
would dilute the completed course learning map and make the entry document grow
with unimplemented work.

### Rank persistent connections first

The roadmap starts with sequential requests on one TCP connection before HTTP
pipelining. It directly extends the existing Content-Length and chunked
framing lessons: an implementation must retain bytes that belong to a later
request and must not use EOF as a message boundary.

Alternative considered: start with routing or static files. Rejected because
those add application behavior without first exercising the connection and
message-boundary distinction established by the course.

### Treat the roadmap as planning, not a specification for implementation

Each roadmap item states its learning value and suggested acceptance direction,
but no code change begins from this document alone. Selecting an item requires
a new OpenSpec change with concrete scope and tasks.

## Risks / Trade-offs

- [Roadmap becomes stale] → Mark all items as exploratory and require a new
  change before implementation.
- [Readers mistake a future item for completed behavior] → Clearly separate
  completed course scope from proposed extensions.
- [The roadmap becomes an oversized backlog] → Keep it to a small, ranked set
  and move detailed implementation decisions into their own changes.
