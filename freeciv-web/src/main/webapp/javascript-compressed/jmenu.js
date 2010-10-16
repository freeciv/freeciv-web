/*
jQuery menu

Example:
$(document).ready(function()
{
    $('#jmenu').jmenu({animation:'fade',duration:100});
});

(c) 2010 Sawanna Team (http://sawanna.org)
*/

var jmenu={
    effect: 'fade',           /* default animation effect */
    duration: 400,         /* default duration */
    set: function (settings)
    {
       try
        {
            if (settings.animation == 'show') { this.effect='show'; }
            if (settings.animation == 'slide') { this.effect='slide'; }
            if (settings.animation == 'fade') { this.effect='fade'; }
        } catch (e) {}
        
        try
        {
            this.duration=settings.duration;
        } catch (e) {} 
    },
    fix_pos:function(elem)
    {
        if ($(elem).parent('ul').parent('li').length)
        {
            $(elem).children('ul').eq(0).css({marginTop:-$(elem).height(),marginLeft:$(elem).width()});
        } else
        {
            $(elem).children('ul').eq(0).css({'top':$(elem).offset().top+$(elem).height()-$(elem).parent().parent().parent().parent().parent().parent().offset().top,
			                      'left':$(elem).offset().left-$(elem).parent().parent().parent().parent().parent().parent().offset().left});
        }
    },
    show:function(elem)
    {
        if (this.effect=='fade') { $(elem).children('ul').eq(0).stop(1,1).fadeIn(this.duration); }
        else if (this.effect=='slide') {$(elem).children('ul').eq(0).stop(1,1).slideDown(this.duration); }
        else if (this.effect=='show') { $(elem).children('ul').eq(0).stop(1,1).show(this.duration); }
    },
    hide: function(elem)
    {
        $(elem).children('ul').eq(0).stop(1,1).fadeOut(100);
    }
}

jQuery.fn.jmenu=function(settings)
{
    jmenu.set(settings);
    
    $(this).find('li').each(function()
    {
            $(this).hover(
                function()
                {
                    jmenu.fix_pos(this);
                    jmenu.show(this);
                },
                function()
                {
                    jmenu.hide(this);
                }
            );
    });
}
