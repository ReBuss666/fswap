_fswap_completion() {
  local cur prev words cword
  _init_completion -s || return

  local opts="-a --atomic -p --perms -u --undo -v --verbose -h --help"

  if [[ "$cur" == -* ]]; then
    COMPREPLY=($(compgen -W "$opts" -- "$cur"))
    return 0
  fi

  _filedir
}

complete -F _fswap_completion fswap
