var projects = [['mosra/m.css', 'master']];

/* Ability to override the projects via query string */
if(location.search) {
    let params = new URLSearchParams(location.search);
    projects = [];
    for(let p of params) projects.push(p);
}

function timeDiff(before, now) {
    var diff = now.getTime() - before.getTime();

    /* Try days first. If less than a day, try hours. If less than an hour, try
       minutes. If less than a minute, say "now". */
    if(diff/(24*60*60*1000) > 1)
        return Math.round(diff/(24*60*60*1000)) + "d ago";
    else if(diff/(60*60*1000) > 1)
        return Math.round(diff/(60*60*1000)) + "h ago";
    else if(diff/(60*1000) > 1)
        return Math.round(diff/(60*1000)) + "m ago";
    else
        return "now";
}

/* https://circleci.com/docs/api/#recent-builds-for-a-single-project */
function fetchLatestCircleCiJobs(project, branch) {
    var req = window.XDomainRequest ? new XDomainRequest() : new XMLHttpRequest();
    if(!req) return;

    req.open('GET', 'https://circleci.com/api/v1.1/project/github/' + project + '/tree/' + branch + '?limit=10&offset=0&shallow=true');
    req.responseType = 'json';
    req.onreadystatechange = function() {
        if(req.readyState != 4) return;

        //console.log(req.response);

        var now = new Date(Date.now());

        /* It's not possible to query just the latest build, so instead we have
           to query N latest jobs and then go as long as they have the same
           commit. Which is kinda silly, but better than going one-by-one like
           with Travis, right? */
        var commit = '';
        for(var i = 0; i != req.response.length; ++i) {
            var job = req.response[i];

            /* Some other commit, we have everything. Otherwise remember the
               commit for the next iteration. */
            if(commit && job['vcs_revision'] != commit)
                break;
            commit = job['vcs_revision'];

            /* If the YML fails to parse, job_name is Build Error. Skip it
               completely to avoid errors down the line. */
            if(job['workflows']['job_name'] == 'Build Error')
                continue;

            var id = job['reponame'].replace("m.css", "mcss") + '-' + job['workflows']['job_name'];
            var elem = document.getElementById(id);
            if(!elem) {
                console.log('Unknown CircleCI job ID', id);
                continue;
            }

            var type;
            var status;
            var ageField;
            if(job['status'] == 'success') {
                type = 'm-success';
                status = '✔';
                ageField = 'stop_time';
            } else if(job['status'] == 'queued' || job['status'] == 'scheduled' || job['status'] == 'not_running') {
                type = 'm-info';
                status = '…';
                ageField = 'queued_at';
            } else if(job['status'] == 'not_running') {
                type = 'm-info';
                status = '…';
                ageField = 'usage_queued_at';
            } else if(job['status'] == 'running') {
                type = 'm-warning';
                status = '↺';
                ageField = 'start_time';
            } else if(job['status'] == 'failed' || job['status'] == 'infrastructure_fail' || job['status'] == 'timedout') {
                type = 'm-danger';
                status = '✘';
                ageField = 'stop_time';
            } else if(job['status'] == 'canceled') {
                type = 'm-dim';
                status = '∅';
                ageField = 'stop_time';
            } else {
                /* retried, not_run, not_running, no_test, fixed -- not sure
                   what exactly these mean */
                type = 'm-default';
                status = job['status'];
                ageField = 'usage_queued_at';
            }

            var age = timeDiff(new Date(Date.parse(job[ageField])), now);

            /* Update the field only if it's not already filled -- in that case
               it means this job got re-run. */
            if(!elem.className) {
                elem.innerHTML = '<a href="' + job['build_url'] + '" title="' + job['status'] + ' @ ' + job[ageField] + '">' + status + '<br /><span class="m-text m-small">' + age + '</span></a>';
                elem.className = type;
            }
        }
    };
    req.send();
}

function fetchLatestCodecovJobs(project, branch) {
    var req = window.XDomainRequest ? new XDomainRequest() : new XMLHttpRequest();
    if(!req) return;

    req.open("GET", 'https://codecov.io/api/gh/' + project + '/branch/' + branch, true);
    req.responseType = 'json';
    req.onreadystatechange = function() {
        if(req.readyState != 4) return;

        //console.log(req.response);

        var repo = req.response['repo']['name'];
        var id = 'coverage-' + repo.replace("m.css", "mcss");
        var elem = document.getElementById(id);

        var commit = req.response['commit'];
        var coverage = (commit['totals']['c']*1.0).toFixed(1);
        var type;
        if(commit['state'] != 'complete') type = 'm-info';
        else if(Math.round(coverage) > 90) type = 'm-success';
        else if(Math.round(coverage) > 50) type = 'm-warning';
        else type = 'm-danger';

        var date = commit['updatestamp'];
        var age = timeDiff(new Date(Date.parse(date)), new Date(Date.now()));

        elem.innerHTML = '<a href="https://codecov.io/gh/mosra/' + repo + '/tree/' + commit['commitid'] + '" title="@ ' + date + '"><strong>' + coverage + '</strong>%<br /><span class="m-text m-small">' + age + '</span></a>';
        elem.className = type;
    };
    req.send();
}

for(var i = 0; i != projects.length; ++i) {
    fetchLatestCircleCiJobs(projects[i][0], projects[i][1]);
    fetchLatestCodecovJobs(projects[i][0], projects[i][1]);
}
