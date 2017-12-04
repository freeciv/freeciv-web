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
        border: 1px solid black;
        padding: 5px;
        font-size: 135%;
        text-transform: capitalize;

    }
</style>

</head>
<body>
	<%@include file="/WEB-INF/jsp/fragments/header.jsp" %>
	<div class="container">
		<h2>Hall Of Fame</h2>
        <br>

        <table>
             <tr>
                <th>Position</td>
                <th>Name</td>
                <th>Nation</td>
                <th>End turn</td>
                <th>End date</td>
                <th>Score</td>
              </tr>
            <c:forEach items="${data}" var="item">
              <tr>
                <td><c:out value="${item.position}"/></td>
                <td><c:out value="${item.username}"/></td>
                <td><c:out value="${item.nation}"/></td>
                <td><c:out value="${item.end_turn}"/></td>
                <td><c:out value="${item.end_date}"/></td>
                <td><c:out value="${item.score}"/></td>
              </tr>
            </c:forEach>
        <table>
        <br><br><br>

		<!-- Site footer -->
		<%@include file="/WEB-INF/jsp/fragments/footer.jsp"%>
	</div> <!-- container -->
</body>
</html>