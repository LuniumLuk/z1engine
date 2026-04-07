## ADDED Requirements

### Requirement: Knowledge base must exist at `openspec/kb/` with a master index

All agent-consumable project knowledge MUST be stored in `openspec/kb/` as markdown topic files with a single `index.md` entry point.

#### Scenario: Index file structure
- **WHEN** an agent reads `openspec/kb/index.md`
- **THEN** it MUST contain a numbered list of all topic files in the format:
  ```
  [ID] filename.md -- one-line description
  ```
- **AND** every `.md` file in `openspec/kb/` (except `index.md`) MUST appear in the index
- **AND** every entry in the index MUST have a corresponding file on disk

#### Scenario: Topic file structure
- **WHEN** a topic file exists in `openspec/kb/`
- **THEN** it MUST start with `# <Title>` (h1)
- **AND** immediately followed by `> Summary: <one-line description>`
- **AND** followed by `> Scope: <files/dirs this topic covers>`
- **AND** the body MUST use only: `##`/`###` headings, tables, bullet lists, fenced code blocks
- **AND** the file MUST NOT contain prose paragraphs (continuous text >1 sentence without structure)

#### Scenario: Topic file granularity
- **WHEN** a topic file exceeds 150 lines
- **THEN** it SHOULD be split into sub-topics with their own files
- **AND** the original file MUST link to sub-topics via `→ see [sub-topic.md]`
- **AND** the index MUST be updated to include the new sub-topic files

### Requirement: FORCE -- All subagents must read KB at session start

Every agent command invocation MUST begin by reading the knowledge base before performing any task-specific work.

#### Scenario: Session start KB load
- **WHEN** any agent command is invoked (`/opsx-propose`, `/opsx-apply`, `/opsx-archive`, `/opsx-explore`)
- **THEN** the agent MUST read `openspec/kb/index.md` as its first action
- **AND** the agent MUST read topic files relevant to the current task (at minimum: `project-map.md` and `coding-style.md`)
- **AND** the agent MUST use KB content as the authoritative reference for project facts
- **AND** this step MUST NOT be skipped even under context pressure

#### Scenario: KB content authority
- **WHEN** KB content conflicts with information in deprecated `docs/*.md` files
- **THEN** the KB content MUST be treated as authoritative
- **AND** the agent SHOULD update the deprecated doc if time permits

### Requirement: FORCE -- All subagents must check and update KB at session end

Every agent command invocation MUST end with a mandatory KB review-and-update step. This step executes unconditionally -- even if the main task failed, was cancelled, or was interrupted.

#### Scenario: Session end KB update (discoveries made)
- **WHEN** an agent session ends (task complete, failed, paused, or interrupted)
- **AND** the agent discovered new information during the session (file patterns, build quirks, API behavior, error resolutions, architectural insights)
- **THEN** the agent MUST compare discoveries against current KB topic files
- **AND** the agent MUST update any stale or incomplete topic files with the new information
- **AND** the agent MUST add new topic files and index entries if discoveries don't fit existing topics
- **AND** the agent MUST log what was updated: `"KB updated: <topic-file> -- <what changed>"`

#### Scenario: Session end KB update (no discoveries)
- **WHEN** an agent session ends
- **AND** no new information was discovered beyond what the KB already contains
- **THEN** the agent MUST log: `"KB verified, no updates needed"`
- **AND** this verification step MUST NOT be skipped

#### Scenario: KB update on failed sessions
- **WHEN** an agent session fails or encounters an error
- **THEN** the KB update step MUST still execute
- **AND** error-related discoveries (build failures, missing dependencies, workarounds found) MUST be captured in the relevant KB topic file (typically `build.md` or `testing.md`)

### Requirement: FORCE instruction must be embedded in all agent command definitions

The KB read-at-start and update-at-end instructions MUST be physically present in every agent command file, not referenced by link.

#### Scenario: Command file contains FORCE block
- **WHEN** any file in `.opencode/command/opsx-*.md` or `.github/prompts/opsx-*.prompt.md` is read
- **THEN** it MUST contain a `## FORCE: Knowledge Base Protocol` section
- **AND** this section MUST appear before the command-specific steps
- **AND** it MUST contain both the session-start read instructions and the session-end update instructions
- **AND** the session-end instruction MUST be marked as `UNCONDITIONAL -- execute even on failure/cancel/interrupt`

#### Scenario: New agent commands inherit FORCE instruction
- **WHEN** a new agent command is created (new `.md` file in `.opencode/command/` or `.github/prompts/`)
- **THEN** it MUST include the `## FORCE: Knowledge Base Protocol` section
- **AND** omitting it is a spec violation that must be caught during review

### Requirement: KB updates must preserve format compliance

All KB modifications MUST maintain the compact doc format defined in `compact-doc-format/spec.md`.

#### Scenario: Agent updates a topic file
- **WHEN** an agent modifies a KB topic file
- **THEN** the update MUST use only permitted structural elements (headings, tables, bullets, code blocks)
- **AND** the update MUST NOT introduce prose paragraphs
- **AND** the `> Summary:` and `> Scope:` blockquotes MUST remain accurate after the edit
- **AND** the file MUST remain under 150 lines (split if update pushes it over)

#### Scenario: Agent adds a new topic file
- **WHEN** an agent creates a new topic file in `openspec/kb/`
- **THEN** it MUST follow the topic file structure (title, summary, scope, structured body)
- **AND** the agent MUST add an entry to `index.md` immediately
- **AND** the index entry MUST use the next available `[ID]` number
