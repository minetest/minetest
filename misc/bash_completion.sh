_luanti() {
  local cur prev opts color_values

  COMPREPLY=()
  cur="${COMP_WORDS[COMP_CWORD]}"
  prev="${COMP_WORDS[COMP_CWORD-1]}"

  opts="--address --color --config --console --debugger --gameid --go --help --info --logfile --map-dir --migrate --migrate-auth --migrate-mod-storage --migrate-players --name --password --password-file --port --quiet --random-input --recompress --run-benchmarks --run-unittests --server --terminal --test-module --trace --verbose --version --world --worldlist --worldname"
  color_values="always never auto"


  if [[ "$prev" == "--color" ]]; then
    COMPREPLY=($(compgen -W "$color_values" -- "$cur"))
  else
    COMPREPLY=($(compgen -W "$opts" -- "$cur"))
  fi
}

complete -F _luanti luanti
