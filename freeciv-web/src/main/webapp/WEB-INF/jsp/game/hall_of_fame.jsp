<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core"%>
<%@ taglib prefix="fn" uri="http://java.sun.com/jsp/jstl/functions"%>
<%@ include file="/WEB-INF/jsp/fragments/i18n.jsp"%>
<!DOCTYPE html>
<html lang="en">
<head>
<%@include file="/WEB-INF/jsp/fragments/head.jsp"%>

<!-- Bootstrap core CSS -->
<link rel="stylesheet" href="/css/morris.css" />
<script src="/javascript/libs/raphael-min.js"></script>
<script src="/javascript/libs/morris.min.js"></script>
<style>
	table, th, td {
        padding: 5px;
        font-size: 135%;
        text-transform: capitalize;
    }

    .score_row {
      border-bottom: 1px solid black;
    }

</style>

</head>
<body>
	<%@include file="/WEB-INF/jsp/fragments/header.jsp" %>


	<div class="container">

		<div class="row">
    			<div class="col-md-12">
    				<script async src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js"></script>
    				<ins class="adsbygoogle"
    					style="display:block"
    					data-ad-client="ca-pub-5520523052926214"
    					data-ad-slot="7043279885"
    					data-ad-format="auto"></ins>
    				<script>
    				(adsbygoogle = window.adsbygoogle || []).push({});
    				</script>
    			</div>
    		</div>
 		<div class="row">
     			<div class="col-md-12">

		<h1>Hall Of Fame</h1>
        <br>

        <table style="width: 100%;">
             <tr>
                <th>Rank:</td>
                <th>Name:</td>
                <th>Nation:</td>
                <th>End turn:</td>
                <th>End date:</td>
                <th>Score:</td>
                <th>Total Score:</td>
              </tr>
            <c:forEach items="${data}" var="item">
              <tr class="score_row">
                <td><c:out value="${item.position}"/></td>
                <td><c:out value="${item.username}"/></td>
                <td><c:out value="${item.nation}"/></td>
                <td><c:out value="${item.end_turn}"/></td>
                <td><c:out value="${item.end_date}"/></td>
                <td><c:out value="${item.score}"/></td>
                <td><c:out value="${item.total_score}"/></td>
              </tr>
            </c:forEach>
        <table>
        <br><br><br>
    			</div>
    		</div>

    		
		<!-- Site footer -->
		<%@include file="/WEB-INF/jsp/fragments/footer.jsp"%>
	</div> <!-- container -->
</body>
</html>