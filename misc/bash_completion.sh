__gameid_list() {
  # FIXME: Why luanti --gameid list returns list into stderr?
  output=($(eval "luanti --gameid list 2>&1"))
  output+=("list")
  eval "$1=(\"${output[@]}\")"
}

__worldname_list() {
  string=$(eval "luanti --worldlist name")
  output=$(awk 'NR>1{print $0}' <<< "$string")
  eval "$1=(\"${output[@]}\")"
}

_luanti() {
  local cur prev opts file_opts color_values worldlist_values

  COMPREPLY=()
  cur="${COMP_WORDS[COMP_CWORD]}"
  prev="${COMP_WORDS[COMP_CWORD-1]}"

  opts="--address --color --config --console --debugger --gameid --go --help --info --logfile --map-dir --migrate --migrate-auth --migrate-mod-storage --migrate-players --name --password --password-file --port --quiet --random-input --recompress --run-benchmarks --run-unittests --server --terminal --test-module --trace --verbose --version --world --worldlist --worldname"
  file_opts="--config --logfile --map-dir --password-file --world"
  color_values="always auto never"
  worldlist_values="both name path"


  if [[ "$prev" == "--color" ]]; then
    COMPREPLY=($(compgen -W "$color_values" -- "$cur"))
  elif [[ "$prev" == "--worldlist" ]]; then
    COMPREPLY=($(compgen -W "$worldlist_values" -- "$cur"))
  elif [[ "$prev" == "--gameid" ]]; then
    local gameid_values
    __gameid_list gameid_values
    COMPREPLY=($(compgen -W "$gameid_values" -- "$cur"))
  elif [[ "$prev" == "--worldname" ]]; then
    local worldname_values
    __worldname_list worldname_values
    COMPREPLY=($(compgen -W "$worldname_values" -- "$cur"))
  elif [[ " ${file_opts[*]} " == *" ${prev} "* ]]; then
    _comp_compgen_filedir
  else
    COMPREPLY=($(compgen -W "$opts" -- "$cur"))
  fi
}

complete -F _luanti luanti
