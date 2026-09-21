<!--
English or Spanish, whichever you prefer. / En inglés o en español, como quieras.
The two house rules are in CONTRIBUTING.md; this is just the short form.
-->

## What this changes

<!-- One paragraph. What was wrong or missing, and what it does now. -->

## What was measured

<!--
"Should be faster" is not a measurement.

  solver --bench --save before
  ...the change...
  solver --bench --vs before

Paste the comparison. If the change cannot affect speed, say so and why.
-->

## The check, and the mutation that proves it

<!--
Which assertion in src/check.hpp guards this, and what you broke on purpose to
watch it go red. A check that does not catch its own mutation is worthless.
-->

- [ ] `solver --check` is green
- [ ] Builds with no warnings (`-Wall -Wextra`, `/W4`)
- [ ] Any new user-facing text exists in **both** languages
