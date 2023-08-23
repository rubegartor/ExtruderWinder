const chart = Highcharts.stockChart('container', {
  chart: {
    animation: false,
  },
  plotOptions: {
    series: {
      color: '#16a085',
      connectNulls: true,
      lineWidth: 4,
      smooth: true
    }
  },
  rangeSelector: {
    inputEnabled: false,
    buttons: [{
      type: 'millisecond',
      count: 10,
      text: '10'
    }, {
      type: 'millisecond',
      count: 50,
      text: '50'
    }, {
      type: 'millisecond',
      count: 100,
      text: '100'
    }, {
      type: 'millisecond',
      count: 1000,
      text: '1000'
    }, {
      type: 'all',
      text: 'All'
    }],
    selected: 2
  },
  yAxis: {
    decimals: 2,
    min: 1.50,
    max: 1.90,
    labels: {
      format: '{value:.2f}'
    },
    plotLines: [{
      value: 1.65,
      color: '#8e44ad',
      width: 1,
      label: {
        text: '1.65'
      },
      zIndex: 3,
    }, {
      value: 1.85,
      color: '#8e44ad',
      width: 1,
      label: {
        text: '1.85'
      },
      zIndex: 3,
    }, {
      value: 1.74,
      color: '#e67e22',
      dashStyle: 'Dash',
      width: 1,
      label: {
        text: '1.75'
      },
      zIndex: 3,
    }],
    tickAmount: 10,
  },
  xAxis: {
    type: 'linear',
    labels: {
      formatter: function () {
        return this.value;
      },
      style: {
        fontSize: '10px'
      }
    }
  },
  exporting: {
    enabled: false
  },
  accessibility: {
    enabled: false
  },
  series: [{
    name: 'Diametro',
    data: []
  }],
  navigator: {
    yAxis: {
      min: 1.50,
      max: 1.90,
    },
    xAxis: {
      labels: {
        enabled: false,
      }
    }
  },
  credits: {
    enabled: false
  }
});

if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener('minRead', function (e) {
    document.getElementById('minRead').innerText = e.data;
  }, false);

  source.addEventListener('maxRead', function (e) {
    document.getElementById('maxRead').innerText = e.data;
  }, false);

  source.addEventListener('lastRead', function (e) {
    document.getElementById('lastRead').innerText = e.data;

    let series = chart.series[0];
    let x = series.data.length + 1,
      y = parseFloat(e.data);
    series.addPoint([x, y], true, false);
  }, false);

  source.addEventListener('extrudedLength', function (e) {
    document.getElementById('extrudedLength').innerText = e.data + 'm';
  }, false);

  source.addEventListener('extrudedWeight', function (e) {
    document.getElementById('extrudedWeight').innerText = parseFloat(e.data).toFixed(2) + 'g';
  }, false);

  source.addEventListener('pullerSpeed', function (e) {
    document.getElementById('pullerSpeed').innerText = e.data;
  }, false);

  source.addEventListener('time', function (e) {
    document.getElementById('time').innerText = millisFormat(e.data);
  }, false);

  source.addEventListener('setPoint', function (e) {
    let setPoint = parseFloat(e.data);

    updateSetPoint(setPoint);
  }, false);

  source.addEventListener('reset', function (_) {
    resetChart();
  }, false);
}

function updateSetPoint(setPoint) {
  chart.yAxis[0].update({
    decimals: 2,
    min: setPoint - 0.2,
    max: setPoint + 0.2,
    plotLines: [{
      value: (setPoint - 0.1).toFixed(2),
      color: '#8e44ad',
      width: 1,
      label: {
        text: (setPoint - 0.1).toFixed(2).toString()
      },
      zIndex: 3,
    }, {
      value: (setPoint + 0.1).toFixed(2),
      color: '#8e44ad',
      width: 1,
      label: {
        text: (setPoint + 0.1).toFixed(2).toString()
      },
      zIndex: 3,
    }, {
      value: setPoint.toFixed(2),
      color: '#e67e22',
      dashStyle: 'Dash',
      width: 1,
      label: {
        text: setPoint.toFixed(2).toString()
      },
      zIndex: 3,
    }],
    tickAmount: 10,
  });

  chart.navigator.yAxis.setExtremes(chart.yAxis[0].getExtremes().min, chart.yAxis[0].getExtremes().max);
}

function onStart() {
  fetch('/onStart', { method: 'GET' })
    .then(response => response.json())
    .then(data => {
      document.getElementById('minRead').innerText = data.min;
      document.getElementById('maxRead').innerText = data.max;

      updateSetPoint(data.setPoint);
    });
}

function resetChart() {
  chart.series[0].setData([]);
  chart.redraw();
}

function millisFormat(millis) {
  const segundos = Math.floor(millis / 1000);
  const [horas, minutos, segundosRestantes] = [
    segundos / 3600,
    (segundos % 3600) / 60,
    segundos % 60
  ].map(Math.floor);

  return `${String(horas).padStart(2, "0")}:${String(minutos).padStart(2, "0")}:${String(segundosRestantes).padStart(2, "0")}`;
}

document.addEventListener("DOMContentLoaded", function () {
  onStart();
});