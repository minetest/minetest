function e(s) {
    if (typeof s === "undefined") s = '';
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;'); //mc"
}

function human_time(t, abs) {
    var n = 's';
    if (!t || t < 0) t = 0;
    var f = 0;
    var s = parseInt(abs ? (t || 0) : (new Date().getTime() / 1000 - (t || 0)));
    if (!s || s <= 0) s = 0;
    if (s == 0) return 'now';
    if (s >= 60) {
        s /= 60;
        n = 'm';
        if (s >= 60) {
            s /= 60;
            n = 'h';
            f = 1;
            if (s >= 24) {
                s /= 24;
                n = 'd';
                f = 1;
                if (s >= 30) {
                    s /= 30;
                    n = 'M';
                    f = 1;
                    if (s >= 12) {
                        s /= 12;
                        n = 'y';
                        f = 1;
                    }
                }
            }
        }
    }
    return ((f ? parseFloat(s).toFixed(1) : parseInt(s)) + n);
}

function success(r) {
    if (!r || !r.list) return;
    var h = '<table class="mts_table"><tr class="mts_head"><th>ip[:port]</th><th>clients/max</th><th>version gameid</th><th>name</th><th>desc</th><th>flags</th><th>uptime</th><th>ping</th></tr>';
    for (var i = 0; i < r.list.length; ++i) {
        var s = r.list[i];
        if (!s) continue;
        h += '<tr class="mts_row">';
        h += '<td class="mts_address">' + e(s.address) + (s.port != 30000 ? (':' + e(s.port)) : '') + '</td>';
        h += '<td class="mts_clients">' + e(s.clients) + (s.clients_max ? '/' + e(s.clients_max) : '') + (s.clients_top ? ', ' + s.clients_top : '') + '</td>';
        h += '<td class="mts_version">' + e(s.version) + ' ' + e(s.gameid) + '</td>';
        h += '<td class="mts_url">';
        if (s.url) h += '<a href="' + e(s.url) + '">';
        h += e(s.name || s.url);
        if (s.url) h += '</a>';
        h += '</td>';
        h += '<td class="mts_description">' + e(s.description) + '</td>';
        h += '<td class="mts_flags">' + e(s.password ? 'Pwd ' : '') + (s.creative ? 'Cre ' : '') + (s.damage ? 'Dmg ' : '') + (s.pvp ? 'Pvp ' : '') + (s.dedicated ? 'Ded ' : '') + '</td>';
        if (!s.time || s.time < 0) s.time = 0;
        if (!s.start || s.start < 0) s.start = 0;
        h += '<td class="mts_time">' + (s.uptime ? human_time(s.uptime, 1) : '') + '</td>';
        h += '<td class="mts_ping">' + (s.ping ? parseFloat(s.ping).toFixed(3) * 1000 : '') + '</td>';
        h += '</tr>';
    }
    h += '</table>'
    jQuery('#table').html(h);
}
var master_root;

function get() {
    jQuery.getJSON((master_root || '') + 'list', success);
    setTimeout(get, 60000);
}
get();
