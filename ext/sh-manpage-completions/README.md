# sh-manpage-completions

Automatically generate bash and zsh completions from man pages.

Uses [fish-shell](https://github.com/fish-shell/fish-shell)'s `create_manpage_completions.py` to parse the pages, then converts the resulting fish completions.

## Example

```
./run.sh /usr/share/man/man1/tar.1.gz
```

You can then take the files from `completions/$shell/` and use them according to your configuration:

- **Bash**: Source them in an initialization file (e.g. `~/.bashrc`)
- **Zsh**: Add them to a directory in `$fpath`

## Limitations

Bash doesn't support descriptions in completions. There has been some [discussion about workarounds](https://stackoverflow.com/questions/7267185/bash-autocompletion-add-description-for-possible-completions). Two different strategies were implemented:

1. Separate descriptions

Consists on printing the completions with descriptions, before bash displays the completions again. It results in redundancy but doesn't break tab behaviour. Descriptions can be omitted like so:

```
BASH_NO_DESCRIPTIONS=1 ./run.sh /usr/share/man/man1/tar.1.gz
```

2. Filter through a selector

You can use a fuzzy selector to extract the right option, containg both the completion and its description. No redundancy, but relies on an external application. Can be used like so:

```
BASH_USE_SELECTOR=1 ./run.sh /usr/share/man/man1/tar.1.gz
```

Uses [fzf](https://github.com/junegunn/fzf) by default. You can pass another one, optionally with an argument that takes the current input as a query, filtering down the results:

```
BASH_USE_SELECTOR=1 SELECTOR=fzy SELECTOR_QUERY='-q' ./run.sh /usr/share/man/man1/tar.1.gz
```

## Related Work

- Built-in parsing of options output by `--help`:
    - **Bash**: `complete -F _longopt`
    - **Zsh**: `compdef _gnu_generic`
- [GitHub \- RobSis/zsh\-completion\-generator: Plugin that generates completion functions automatically from getopt\-style help texts](https://github.com/RobSis/zsh-completion-generator)
- [GitHub \- Aloxaf/fzf\-tab: Replace zsh&#39;s default completion selection menu with fzf!](https://github.com/Aloxaf/fzf-tab)

## License

Files in `fish-tools` were taken from fish-shell's source code, which is released under the GNU General Public License, version 2 (see LICENSE.GPL2).

The remaining source code is released under the MIT License (see LICENSE.MIT).
