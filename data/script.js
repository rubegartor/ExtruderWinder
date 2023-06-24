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
    max: 1.92,
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
  var source = new EventSource('http://10.2.0.188/events');

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

    console.log('Setpoint: ' + setPoint);

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
  }, false);
}

function millisFormat(millis) {
  const segundos = Math.floor(millis / 1000);
  const minutos = Math.floor(segundos / 60);
  const horas = Math.floor(minutos / 60);

  const tiempoEnHoras = horas.toString().padStart(2, "0");
  const tiempoEnMinutos = (minutos % 60).toString().padStart(2, "0");
  const tiempoEnSegundos = (segundos % 60).toString().padStart(2, "0");

  return `${tiempoEnHoras}:${tiempoEnMinutos}:${tiempoEnSegundos}`;
}