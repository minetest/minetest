_luanti() {
  local cur prev opts

  COMPREPLY=()
  cur="${COMP_WORDS[COMP_CWORD]}"
  prev="${COMP_WORDS[COMP_CWORD-1]}"

  opts="--address --color --config --console --debugger --gameid --go --help --info --logfile --map-dir --migrate --migrate-auth --migrate-mod-storage --migrate-players --name --password --password-file --port --quiet --random-input --recompress --run-benchmarks --run-unittests --server --terminal --test-module --trace --verbose --version --world --worldlist --worldname"


  COMPREPLY=($(compgen -W "$opts" -- "$cur"))
}

complete -F _luanti luanti
