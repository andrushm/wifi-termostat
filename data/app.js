/**
 * Created by andrush on 21.12.2017.
 */
var app = {
    url: "status.json",
    // 1: On, 0: Off
    statusClass: {0:"danger", 1:"success"},
    heatingStatusClass: {0:"danger", 1:"success", 2:"warning"},

    statusContainer: "#switcher",
    heatingStatusContainer: "#ajax-statusText",
    schedulesContainer: "#schedules tbody",

    init: function () {
        this.getStatuses();
    },
    getStatuses: function () {

        $.ajax(this.url, {
            method: "GET",
            // data: paramsString,
            // statusCode: this._statusCodeCallbacks,
            success: function(data){
                // console.log(data);
                $.each(data, function (key, value) {
                    // console.log(key, value);
                    $itemContainer = $('#ajax-' + key);
                    // console.log($itemContainer);
                    if ($itemContainer.length > 0)
                    {
                        $itemContainer.html(value);
                    }
                });

                $(app.statusContainer).removeClass('btn btn-success btn-danger');
                $(app.statusContainer).addClass('btn btn-' + app.statusClass[data.status]);

                $(app.heatingStatusContainer).removeClass('label label-success label-warning');
                $(app.heatingStatusContainer).addClass('label label-' + app.heatingStatusClass[data.heatingStatus]);

                if (data.schedules.length > 0)
                {
                    $.each(data.schedules, function (key, schedule) {
                        var $schedulesContainer = $(app.schedulesContainer);
                        var $scheduleObject = $('<tr></tr>');
                        $scheduleObject.attr('id', 'schedule-' + schedule.id);
                        $scheduleObject.html('<td>' + schedule.id + '</td><td>' + schedule.enabled
                            + '</td><td>' + schedule.begin + '</td><td>' + schedule.end + '</td><td>' + schedule.set + '</td>');
                        $schedulesContainer.append($scheduleObject);
                       console.log(schedule);
                    });
                }

                // console.log(app.heatingStatusClass[data.heatingStatus]);
                console.log('[app.ajaxTools._loadUrl][success]');
            },
            error: function(data){
                console.log('[app.ajaxTools._loadUrl][error]');
                // app.ajaxTools._flags.isLoading = false;
                // app.ajaxTools._flags.isErrorState = true;
                // app.ajaxTools._afterNavigateError(data.responseText, url, historyState);
            }
        });
    }
};

$().ready(function () {
    console.log('hi');
    app.init();
});

