<!---
SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# AI-Assisted Contributions Policy

Code or other content generated in whole or in part using AI tools can be contributed to Zrythm. AI-generated contributions are held to the same standards as any other contribution, but there are some unique considerations related to AI-generated content that developers should keep in mind.

## Legal

- All code must be compatible with **AGPL-3.0-or-later** with appropriate SPDX headers (see `CONTRIBUTING.md`)
- AI tool terms must not restrict output usage in ways incompatible with Zrythm's license
- If AI output contains third-party copyrighted material (including publicly available code), the contributor must have permission to use and contribute it, with proper attribution and license information

## Developer Certificate of Origin

AI agents **MUST NOT** add `Signed-off-by` tags. Only humans can legally certify the [Developer Certificate of Origin (DCO)](https://developercertificate.org/).

The human submitter is responsible for:

- Reviewing all AI-generated code
- Ensuring compliance with licensing requirements
- Adding their own `Signed-off-by` tag to certify the DCO
- Taking full responsibility for the contribution

## Transparency

AI-assisted contributions should include an `Assisted-by` tag in the git commit message, specifying the AI model used. For example:

```
Assisted-by: Kimi K2.6
```
