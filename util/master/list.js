var master;
if (!master) {
	master = {
		url: "http://servers.minetest.net/list",
		output: "#server_list"
	};
}

function humanTime(seconds) {
	if (!seconds) return '?';
	var conv = {
		y: 31536000,
		d: 86400,
		h: 3600,
		m: 60
	}
	for (var i in conv) {
		if (seconds >= conv[i]) {
			return (seconds / conv[i]).toFixed(1) + i;
		}
	}
}

function escapeHTML(str) {
	return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

function addressString(server) {
	var isIPv6 = server.address.indexOf(":") != -1;
	var addrStr = (isIPv6 ? '[' : '') +
		escapeHTML(server.address) +
		(isIPv6 ? ']' : '');
	var shortStr = addrStr;
	addrStr += ':' + server.port;
	var str = '<span'
	if (shortStr.length > 25) {
		shortStr = shortStr.substr(0, 23) + "&hellip;";
		str += ' class="tooltip" title="' + addrStr + '"'
	}
	if (server.port != 30000)
		shortStr += ':' + server.port;
	return str + '>' + shortStr + '</span>';
}

function tooltipString(str, maxLen) {
	str = escapeHTML(str);
	var shortStr = str;
	var ret = '<span';
	if (shortStr.length > maxLen) {
		shortStr = shortStr.substr(0, maxLen - 2) + "&hellip;";
		ret += ' class="tooltip" title="' + str + '"';
	}
	return ret + '>' + shortStr + '</span>';
}

function hoverList(name, list) {
	if (!list || list.length == 0) return '';
	var str = '<div class="hover_list">'
	str += name + '(' + list.length + ')<br />';
	for (var i in list) {
		str += escapeHTML(list[i]) + '<br />';
	}
	return str + '</div>';
}

function draw(json) {
	html = window.render.servers(json);
	jQuery(master.output || '#server_list').html(html);
}

function get() {
	jQuery.getJSON(master.url, draw);
}

if (!master.no_refresh) {
	setInterval(get, 60 * 1000);
}

get();

