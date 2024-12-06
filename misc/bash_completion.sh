_luanti() {
  local cur prev opts color_values file_opts

  COMPREPLY=()
  cur="${COMP_WORDS[COMP_CWORD]}"
  prev="${COMP_WORDS[COMP_CWORD-1]}"

  opts="--address --color --config --console --debugger --gameid --go --help --info --logfile --map-dir --migrate --migrate-auth --migrate-mod-storage --migrate-players --name --password --password-file --port --quiet --random-input --recompress --run-benchmarks --run-unittests --server --terminal --test-module --trace --verbose --version --world --worldlist --worldname"
  file_opts="--config --logfile --map-dir --password-file --world"
  color_values="always auto never"


  if [[ "$prev" == "--color" ]]; then
    COMPREPLY=($(compgen -W "$color_values" -- "$cur"))
  elif [[ " ${file_opts[*]} " == *" ${prev} "* ]]; then
    _comp_compgen_filedir
  else
    COMPREPLY=($(compgen -W "$opts" -- "$cur"))
  fi
}

complete -F _luanti luanti
