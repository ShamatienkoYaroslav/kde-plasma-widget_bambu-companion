# Contributing

## Development process

This project is developed via [OpenSpec](https://github.com/Fission-AI/OpenSpec),
one change at a time. Before making any non-trivial change:

1. Check [`PLAN.md`](./PLAN.md) for the current phase and its task list.
2. Propose the change: `openspec new change "<name>"` (or, with an AI coding
   agent set up for this repo, `/opsx:propose "<task description>"`), then
   fill in `proposal.md`, spec deltas, `design.md`, and `tasks.md` under
   `openspec/changes/<change-id>/`.
3. Implement the tasks (`/opsx:apply` or manually, checking off `tasks.md` as
   you go).
4. Once implemented and tested, archive the change (`/opsx:archive`) to fold
   its spec deltas into `openspec/specs/`.

See [`CLAUDE.md`](./CLAUDE.md) for architecture, build/test commands, and
project conventions (secrets via KWallet, TOFU certificate pinning for LAN,
etc.).

## Testing expectations

- `protocol/` and `core/` logic: unit tests (Qt Test) against fixture data,
  runnable via `ctest` — required for any change to this code.
- `transport/` and UI work: verified manually in a live Plasma session against
  a real printer (and, for Bambu Cloud, a real account) — not automatable in
  CI. Describe your manual verification steps in the change's `tasks.md`.

## Code style

Follow the conventions already present in the codebase (KDE Frameworks
coding style: `.h`/`.cpp` pairs, `m_` member prefixes, `QML_ELEMENT` for
QML-exposed types). No enforced formatter yet — keep changes consistent with
surrounding code.
