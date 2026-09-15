# Writing Doxygen blocks for JS8Call-improved

This guide describes the repository's Doxyfile (`.github/workflows/misc/Doxyfile`) and the
project's documentation conventions. Read this before writing a block, and run the check at the
end before submitting one.

Commands are written with `@`, not `\`, throughout the tree. Keep to that, and do not use `@fn`:
it misbehaves under 1.16.

## The settings that generate almost all the warnings

-   **`JAVADOC_AUTOBRIEF = NO` and `QT_AUTOBRIEF = NO`.**
    The first sentence of a block is *not* picked up as the brief. A block that opens with prose
    and no `@brief` documents the entity with no summary at all, so every block here carries an
    explicit `@brief`.

-   **`WARN_IF_UNDOCUMENTED = YES` with `EXTRACT_ALL = NO`.**
    Every entity doxygen extracts and finds undocumented produces a warning. The extraction
    settings below decide what is extracted, and an override can inherit its base declaration's
    block instead of carrying one, so not every undocumented entity warns. The tree already emits
    a large backlog that has nothing to do with your change: you are responsible for the warnings
    your diff causes. Capture the log before and after (below) and investigate the difference - do
    not try to clear the whole file.

-   **`WARN_IF_INCOMPLETE_DOC = YES` with `WARN_NO_PARAMDOC = NO`.**
    Omitting all parameter documentation is allowed, but once a block documents parameters it
    must document every parameter. A `@param` naming an argument that is not in the signature - a
    stale name after a refactor is the usual cause - is a documentation error and warns too.
    Parameter completeness and return documentation are separate checks. As a project convention,
    document every non-`void` return value; a `void` function has no `@return` entry.

-   **`EXTRACT_PRIVATE = NO`, `EXTRACT_PRIV_VIRTUAL = NO`, `EXTRACT_STATIC = NO`.**
    Private members, private virtuals and file-static functions are not extracted, so they do not
    warn and a block on one never reaches the output. Document them only where a maintainer
    genuinely needs the explanation. `EXTRACT_LOCAL_METHODS = NO` is set as well, but it governs
    Objective-C implementation methods and so does not apply to this tree.

`WARN_AS_ERROR = NO`, so the docs build will not fail on any of this. Nobody is going to catch it
for you; check the log yourself.

## What a block looks like here

At the top of every new file, named to match the file exactly, case included - a `@file` carrying
the wrong name is a warning:

```cpp
/**
 * @file ActivityDB.h
 * @brief Declares the SQLite store behind activity.db3 (issue #267).
 */
```

On a class, a `@brief` of one line, then as many plain paragraphs as the design needs - why it
exists, what it guarantees, what it deliberately does not do:

```cpp
/**
 * @brief Persistent per-band activity storage (activity.db3).
 *
 * Stores the Call Activity table and the RX text history in a dedicated SQLite
 * database, keyed by (configuration, band, callsign) so that activity heard on
 * one band can never be attributed to another (issue #267).
 *
 * Writes happen as activity arrives, not at shutdown, so a crash or power loss
 * loses at most the in-flight row rather than everything since the last clean
 * close.
 */
```

On a function, a `@brief`, every parameter if any parameter is documented, and a useful return
description for a non-`void` result:

```cpp
/**
 * @brief Copies one configuration's stored activity onto another id.
 * @param fromId  The configuration id to copy from.
 * @param toId    The configuration id to copy onto.
 * @return true if both inserts committed.
 */
```

Put the block immediately before the declaration, and leave out the structural command naming it:
`@fn`, `@class`, `@var` and the rest are for blocks that sit elsewhere. `@file` is the exception.

A named namespace needs a block too, since doxygen documents a member only if the namespace
holding it is documented. The anonymous namespace is exempt.

Because `INHERIT_DOCS = YES`, an undocumented override can inherit the base declaration's
documentation. Do not restate it; document an override only where its behaviour genuinely differs.

Because `DISTRIBUTE_GROUP_DOC = NO`, documentation attached to one member is not distributed to
the other members in its member group. Give each member its own block. Separately, assign module
membership with `@ingroup` or by placing the declaration inside an `@{` and `@}` group range;
membership does not require physically moving the declaration. New groups are defined in
`docs/defines.dox`; source files only join one.

An addition to the API gets `@note API x.y+` in its block, where the version is the next release,
and `docs/API.md` is updated in the same PR.

Markdown works inside blocks, and `AUTOLINK_SUPPORT` turns names that resolve into links. A name
that does not resolve is silently left as plain text rather than warning, so wrap anything you do
not want linked - and anything you are unsure of - in backticks.

That holds inside a Markdown table as well: a bare `@name` in a cell is read as a command and
warns, while backticks, `\@` and `@@` each silence it and render a plain at-sign. An HTML entity
does not - `&#64;` reaches the page unrendered.

## How much to write

Write for the maintainer who has to change the code: the brief, the contract a caller has to
honour, and any constraint the signature does not show. Not the implementation, not the reasoning
behind it. A block longer than the function is hard obstructs as much as the inline comments it
replaced.

## Checking before you submit

CI builds the docs with doxygen **1.16.1**, fetched prebuilt by `build-and-deploy-docs.yml`. A
distro package may report a different set of warnings, so check its version first and account for
the version when investigating a discrepancy.

Define this helper in a shell. It reads the complete configuration, disables every output generator
and Graphviz, and optionally narrows `INPUT`:

```bash
doxy_warnings()
(
    cd "$1" || exit
    {
        cat .github/workflows/misc/Doxyfile
        echo "PROJECT_NUMBER=local"
        echo "GENERATE_HTML=NO"
        echo "GENERATE_LATEX=NO"
        echo "GENERATE_XML=NO"
        echo "GENERATE_RTF=NO"
        echo "GENERATE_MAN=NO"
        echo "GENERATE_DOCBOOK=NO"
        echo "GENERATE_PERLMOD=NO"
        echo "GENERATE_SQLITE3=NO"
        echo "HAVE_DOT=NO"
        if [ -n "${2-}" ]; then
            echo "INPUT=$2"
        fi
    } | doxygen -
)

doxygen --version
doxy_warnings . 2> doxy-warnings.log
```

The first line of the log is always `warning: No output formats selected!`. That is the helper
doing its job - it has turned every generator off - not something in the tree. Ignore it.

Run the same command on committed revisions in separate clean worktrees and compare the logs. Do
not switch a worktree that contains edits: Git can carry those edits across the switch and spoil
the baseline. For example, after creating worktrees at `/tmp/js8-master` and `/tmp/js8-change`:

```bash
doxy_warnings /tmp/js8-master 2> /tmp/doxy-before.log
doxy_warnings /tmp/js8-change 2> /tmp/doxy-after.log
sed "s|/tmp/js8-master/||" /tmp/doxy-before.log > /tmp/doxy-before.rel
sed "s|/tmp/js8-change/||" /tmp/doxy-after.log  > /tmp/doxy-after.rel
diff /tmp/doxy-before.rel /tmp/doxy-after.rel
```

Strip each worktree's own prefix before the diff, as above. Doxygen reports absolute paths, so
the two logs share no line at all until you do, and the diff comes back one hundred per cent
changed. This is the step that makes the comparison worth running: on a tree that emits roughly
two thousand warning lines to begin with, an unnormalised diff tells you nothing.

Investigate each remaining difference by cause. A changed line number can make an existing
diagnostic look new, and a change can expose a regression reported in a file that the diff did
not touch.

While iterating on a single file you can narrow the input, which is much faster:

```bash
doxy_warnings . "JS8_Main/ActivityDB.h JS8_Main/ActivityDB.cpp" 2>&1
```

A narrowed run reports references it cannot resolve because the rest of the tree is missing, so
use it to iterate and the full run above to decide you are done.

The full docs build only runs on a PR labelled `documentation`, or on a manual dispatch of
`build-and-deploy-docs.yml`. Do not rely on it to tell you about a problem you could have seen
locally.

## A note if you are an AI agent reading this

The three failures that actually happen, in order: omitting `@brief` because the prose reads like
one; documenting two of three parameters; and inventing a command or a tag that this Doxyfile does
not define. Write the block, run the warning-only command, and investigate every diagnostic change
your diff causes. Do not start rewriting unrelated documentation because the log mentions it; that
backlog predates you, and a diff that grows into it will be rejected on scope.
