## ADDED Requirements

### Requirement: All KB files must use the compact document format

Every file in `openspec/kb/` MUST follow the compact doc format to maximize information density and minimize token consumption.

#### Scenario: Document structure
- **WHEN** a KB topic file is created or updated
- **THEN** it MUST follow this structure:
  ```
  # <Title>
  > Summary: <one-line description of what this topic covers>
  > Scope: <comma-separated list of dirs/files this topic is authoritative for>

  ## <Section>
  <structured content: tables, bullets, code blocks>

  ## <Section>
  ...
  ```
- **AND** no content may appear before the `# Title` heading
- **AND** `> Summary:` MUST be on the line immediately after the title
- **AND** `> Scope:` MUST be on the line immediately after the summary

### Requirement: Only structured elements are permitted in body content

#### Scenario: Permitted elements
- **WHEN** body content is written in a KB file
- **THEN** only these markdown elements are permitted:
  - `##` section headings (level 2)
  - `###` subsection headings (level 3)
  - Bullet lists (`-` prefix)
  - Tables (`| col | col |` format)
  - Fenced code blocks (` ``` ` with language tag)
  - Inline code backticks for identifiers
  - Cross-references: `→ see [filename.md]` or `→ see [filename.md#section]`
  - Bold (`**text**`) for emphasis on key terms only

#### Scenario: Prohibited elements
- **WHEN** body content is written in a KB file
- **THEN** the following MUST NOT appear:
  - Prose paragraphs (continuous text >1 sentence without structural markup)
  - Heading levels deeper than `###` (no `####` or below)
  - HTML tags (no `<!-- comments -->`, no `<details>`, no `<br>`)
  - Images or external links (KB is self-contained)
  - Emojis or decorative characters

### Requirement: Cross-references must use consistent linking format

#### Scenario: Referencing another topic
- **WHEN** a KB topic references information in another KB topic file
- **THEN** it MUST use the format: `→ see [filename.md]`
- **AND** for section-specific references: `→ see [filename.md#section-name]`
- **AND** the referenced file MUST exist in `openspec/kb/`

### Requirement: Index file must follow a strict numbered format

#### Scenario: Index entry format
- **WHEN** `openspec/kb/index.md` is created or updated
- **THEN** each entry MUST use the format:
  ```
  [NN] filename.md -- one-line description
  ```
- **AND** `[NN]` is a zero-padded two-digit sequential ID starting from `[01]`
- **AND** entries MUST be sorted by ID
- **AND** one-line descriptions MUST be under 80 characters

#### Scenario: Index completeness
- **WHEN** the index is read
- **THEN** every `.md` file in `openspec/kb/` (except `index.md`) MUST have an index entry
- **AND** every index entry MUST have a corresponding file on disk
- **AND** orphaned entries or orphaned files are a spec violation

### Requirement: Topic files must stay within line budget

#### Scenario: Line limit
- **WHEN** a KB topic file is created or updated
- **THEN** it SHOULD be 150 lines or fewer
- **AND** if a topic naturally exceeds 150 lines, it MUST be split:
  - The original file keeps the high-level overview
  - Detail sections move to new files (e.g., `render-pipeline.md` splits off `render-pipeline-deferred.md`)
  - The original file links to sub-topics via `→ see [sub-topic.md]`
  - All new files MUST be added to `index.md`

### Requirement: Facts must be stated exactly once

#### Scenario: No duplication across topics
- **WHEN** the same fact could appear in multiple topic files
- **THEN** it MUST be stated in the most specific topic file
- **AND** other topics MUST cross-reference it: `→ see [authoritative-topic.md#section]`
- **AND** if a fact is found duplicated, the less-specific copy MUST be replaced with a cross-reference

### Requirement: Every content element must carry information

#### Scenario: No filler content
- **WHEN** content is written in a KB file
- **THEN** every line MUST convey a concrete fact, command, path, rule, or relationship
- **AND** lines that merely introduce or summarize other lines MUST be removed
- **AND** examples: no "Below is a list of...", no "The following sections describe...", no "As mentioned above..."
