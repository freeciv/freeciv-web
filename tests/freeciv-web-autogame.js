/********************************************************************** 
 Autogame tests for Freeciv-web.
 Use publite2/pubscript_autogame.serv for testing.
***********************************************************************/
casper.on('remote.message', function(message) {
    this.echo('JavaScript console: ' + message);
});

casper.options.waitTimeout = 60 * 60 * 1000;
casper.options.viewportSize = {width: 1024, height: 768};

casper.test.begin('Test starting new Freeciv-web autogame', 3, function suite(test) {
    casper.start("http://localhost/webclient/?action=new");

    casper.waitForSelector('#username_req', function() {
        test.assertHttpStatus(200);
        test.assertTitleMatch(/Freeciv-web/, 'Freeciv-web title is present');
    });

    casper.then(function() {
      this.echo("Filling in username in new game dialog.");
      this.sendKeys('#username_req', 'CasperJS', {reset : true});
      this.echo("Starting Freeciv-web autogame for 200 turns! (this can take a long time! 30 minutes+)");
    });

    casper.thenEvaluate(function() {
      /* Starting new game automatically from Javascript.*/
      dialog_close_trigger = "button";
      if (validate_username()) {
        $("#dialog").dialog('close');
        setTimeout("send_message('/ai CasperJs');", 4000);
        setTimeout("send_message('/start');", 4200);
        setInterval("eval(\"console.log('Running autogame... Current turn: ' + game_info['turn'] + ' - chat messages: ' + chatbox_text);\");", 30000);
      }
    });

    casper.waitForText("T200", function() {
      //this.clickLabel('Ok');
      this.echo("Captured screenshot to be saved as screenshot-autogame.png");
      this.capture('screenshot-autogame.png', undefined, {
        format: 'png',
        quality: 100 
      });
     });

     casper.then(function() {
      this.echo("Checking number of turns completed.");

      test.assertEval(function() {
            return game_info['turn'] >= 200;
      }, "Checks that 200 turns has past.");

    });
 
    casper.run(function() {
        test.done();
    });

 });
