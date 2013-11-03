var master_root, output_to;
var master;
if (!master) master = {
    root: master_root,
    output: output_to
};

function e(s) {
    if (typeof s === "undefined") s = '';
    if (typeof s === "number") return s;
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
    var h = '';
    if (!master.no_total && r.total && r.total_max)
        h += '<div class="mts_total">Players: ' + r.total.clients + ('/' + r.total_max.clients) + ' servers: ' + r.total.servers + ('/' + r.total_max.servers) + '</div>';
    h += '<table class="mts_table">';
    if (r.list.length) {
        h += '<tr class="mts_head">';
        if (!master.no_address) h += '<th>ip[:port]</th>';
        if (!master.no_clients) h += '<th>players/max</th>';
        if (!master.no_version) h += '<th>version gameid mapgen</th>';
        if (!master.no_name) h += '<th>name</th>';
        if (!master.no_description) h += '<th>description</th>';
        if (!master.no_flags) h += '<th>flags</th>';
        if (!master.no_uptime) h += '<th>uptime age</th>';
        if (!master.no_ping) h += '<th>ping</th>';
        h += '</tr>';
    }
    var count = 0;
    for (var i = 0; i < r.list.length; ++i) {
        if (++count > master.limit && master.limit) break;
        var s = r.list[i];
        if (!s) continue;
        if (master.clients_min && s.clients < master.clients_min) continue;
        if (/:/.test(s.address)) s.address = '[' + s.address + ']';
        h += '<tr class="mts_row">';
        if (!master.no_address) h += '<td class="mts_address">' + e(s.address) + (s.port != 30000 ? (':' + e(s.port)) : '') + '</td>';
        if (!master.no_clients) {
            h += '<td class="mts_clients' + (s.clients && s.clients_list ? ' mts_is_clients' : '') + '">';
            if (!master.no_clients_list && s.clients && s.clients_list) {
                h += '<div class="mts_clients_list">Players (' + e(s.clients) + '):<br/>';
                for (var ii in s.clients_list)
                    h += e(s.clients_list[ii]) + '<br/>';
                h += '</div>';
            }
            h += e(s.clients) + (s.clients_max ? '/' + e(s.clients_max) : '') + (s.clients_top ? ', ' + e(s.clients_top) : '') + '</td>';
        }
        var mods = 0;
        if (s.mods && jQuery.isArray(s.mods))
            mods = s.mods.length;
        if (!master.no_version) {
            h += '<td class="mts_version' + (mods ? ' mts_is_mods' : '') + '">' + e(s.version) + ' ' + e(s.gameid) + ' ' + e(s.mapgen);
            if (!master.no_mods && mods) {
                h += '<div class="mts_mods">Mods (' + mods + '):<br/>';
                for (var ii in s.mods)
                    h += e(s.mods[ii]) + '<br/>';
                h += '</div>';
            }
            h += '</td>';
        }
        if (!master.no_name) {
            h += '<td class="mts_url">';
            if (s.url) h += '<a href="' + e(s.url) + '">';
            h += e(s.name || s.url);
            if (s.url) h += '</a>';
            h += '</td>';
        }
        if (!master.no_description) h += '<td class="mts_description">' + e(s.description) + '</td>';
        if (!master.no_flags) {
            h += '<td class="mts_flags">' +
                (s.password ? 'Pwd ' : '') +
                (s.creative ? 'Cre ' : '') +
                (s.damage ? 'Dmg ' : '') +
                (s.pvp ? 'Pvp ' : '') +
                (s.dedicated ? 'Ded ' : '') +
                (s.rollback ? 'Rol ' : '') +
                (s.liquid_finite ? 'Liq ' : '') +
                '</td>';
        }
        if (!s.start || s.start < 0) s.start = 0;
        if (!master.no_uptime) h += '<td class="mts_uptime">' + (s.uptime ? human_time(s.uptime, 1) : s.start ? human_time(s.start) : '') + (s.game_time ? ' ' + human_time(s.game_time, 1) : '') + '</td>';
        if (!master.no_ping) h += '<td class="mts_ping">' + (s.ping ? parseFloat(s.ping).toFixed(3) * 1000 : '') + '</td>';
        h += '</tr>';
    }
    h += '</table>';
    if (master.clients_min || master.limit)
        h += '<a href="#" onclick="delete master.limit;delete master.clients_min; get(1);">more...</a>';
    jQuery(master.output || '#servers_table').html(h);
}

function get(refresh) {
    jQuery.getJSON((master.root || '') + 'list', success);
    if (!refresh && !master.no_refresh) setTimeout(get, 60000);
}
get();
